#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dlfcn.h>

#include "interface.h"

#define NUM_OF_RANDOM_TESTS 10000

// Тип для функции получения реализации
typedef hash_implementation* (*get_implementation_func_t)(void);

// Структура для загруженной библиотеки
typedef struct {
    void* handle;
    hash_implementation* impl;
    const char* name;
} loaded_library;

// Загрузка одной библиотеки
int load_library(const char* lib_path, loaded_library* lib) {
    lib->handle = dlopen(lib_path, RTLD_LAZY);
    if (!lib->handle) {
        fprintf(stderr, "Failed to load %s: %s\n", lib_path, dlerror());
        return -1;
    }

    // Ищем функцию получения реализации
    get_implementation_func_t get_impl = 
        (get_implementation_func_t)dlsym(lib->handle, "get_hash_implementation");
    
    if (!get_impl) {
        fprintf(stderr, "Failed to find get_hash_implementation in %s: %s\n", 
                lib_path, dlerror());
        dlclose(lib->handle);
        return -1;
    }

    lib->impl = get_impl();
    lib->name = lib_path;
    
    printf("Loaded: %s\n", lib_path);
    return 0;
}

// Выгрузка библиотеки
void unload_library(loaded_library* lib) {
    if (lib->handle) {
        dlclose(lib->handle);
        lib->handle = NULL;
    }
}

// Тестирование одной реализации на одном файле
test_result test_implementation(hash_implementation* impl, const char* filename, int hash_size) {
    test_result result = {0};
    result.implementation = impl->name;
    result.test_file = filename;
    result.status = -1;

    // Открываем тестовый файл
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open test file");
        return result;
    }

    // Определяем размер файла
    fseek(file, 0, SEEK_END);
    result.data_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Читаем данные
    uint8_t* data = malloc(result.data_size);
    if (!data) {
        perror("Memory allocation failed");
        fclose(file);
        return result;
    }

    if (fread(data, 1, result.data_size, file) != result.data_size) {
        fprintf(stderr, "Failed to read test file: %s\n", filename);
        free(data);
        fclose(file);
        return result;
    }
    fclose(file);

    // Инициализируем контекст
    void* context = NULL;
    int (*init_func)(void**) = (hash_size == 256) ? impl->init_256 : impl->init_512;
    
    if (init_func(&context) != 0) {
        fprintf(stderr, "Failed to initialize %s\n", impl->name);
        free(data);
        return result;
    }

    
    // Обрабатываем данные
    if (impl->update(context, data, result.data_size) != 0) {
        fprintf(stderr, "Update failed for %s\n", impl->name);
        free(data);
        impl->cleanup(context);
        return result;
    }

    // Получаем хеш
    uint8_t* hash = (hash_size == 256) ? result.hash_256 : result.hash_512;
    if (impl->final(context, hash) != 0) {
        fprintf(stderr, "Final failed for %s\n", impl->name);
        free(data);
        impl->cleanup(context);
        return result;
    }
    
    // Очистка
    impl->cleanup(context);
    free(data);
    result.status = 0;
    
    return result;
}

// Сравнение двух результатов
void compare_results(test_result* result1, test_result* result2, int hash_size) {    
    const uint8_t* hash1 = (hash_size == 256) ? result1->hash_256 : result1->hash_512;
    const uint8_t* hash2 = (hash_size == 256) ? result2->hash_256 : result2->hash_512;
    
    // Сравниваем хеши
    if (memcmp(hash1, hash2, hash_size == 256 ? 32 : 64) != 0) {
        printf("❌ HASHES DIFFER - Stribog-%d\n", hash_size);
        printf("  %s: ", result1->implementation);
        for (int i = 0; i < (hash_size == 256 ? 32 : 64); i++) printf("%02x", hash1[i]);
        printf("\n");
        printf("  %s: ", result2->implementation);
        for (int i = 0; i < (hash_size == 256 ? 32 : 64); i++) printf("%02x", hash2[i]);
        printf("\n");
        exit(1);
    }
    
}

void run_compare(loaded_library *loaded_libs, int num_libs) {
    const char *filename = "../test_data/random_random_size.bin";
    test_result *results[10] = {NULL};
        
    // Тестируем каждую реализацию
    for (int lib_idx = 0; lib_idx < num_libs; lib_idx++) {
        hash_implementation *impl = loaded_libs[lib_idx].impl;

        // Тестируем оба размера хеша
        test_result result_256 = test_implementation(impl, filename, 256);
        test_result result_512 = test_implementation(impl, filename, 512);
        
        // Сохраняем результаты
        if (result_256.status == 0 && result_512.status == 0) {
            results[lib_idx] = malloc(sizeof(test_result));
            *results[lib_idx] = result_256; // Сохраняем 256-битный результат
            memcpy(results[lib_idx]->hash_512, result_512.hash_512, 64);
        }
    }
    
    for (int j = 1; j < num_libs; j++){
        // Сравниваем результаты
        if (results[0] && results[1]) {
            compare_results(results[0], results[j], 256);
            compare_results(results[0], results[j], 512);
        }
    }
    
    // Очистка
    for (int j = 0; j < num_libs; j++) {
        free(results[j]);
    }
}

// Украшательства
void print_progress_bar(int current, int total) {
    int bar_width = 80;  // ширина прогресс-бара в символах
    float ratio = (float)current / total;
    int pos = (int)(bar_width * ratio);

    printf("\r[");  // возврат в начало строки и открывающая скобка
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) printf("#");  // заполненный прогресс
        else printf(" ");           // пустое пространство
    }
    printf("] %3d%% (%d/%d)", (int)(ratio * 100), current, total);  // показываем процент
    fflush(stdout);  // вывод без буферизации
}


int main() {
    // Инициализация генератора случайных чисел
    srand((unsigned int)time(NULL));

    printf("=== Stribog Implementation Comparator ===\n\n");

    // Список библиотек для загрузки
    const char* libraries[] = {
        "../libs/libmy_impl.so",
        "../libs/libother_impl_1.so", 
        "../libs/libother_impl_2.so",
        "../libs/libother_impl_3.so",
        NULL
    };

    // Загружаем библиотеки
    loaded_library loaded_libs[10];
    int num_libs = 0;
    
    for (int i = 0; libraries[i] != NULL; i++) {
        if (load_library(libraries[i], &loaded_libs[num_libs]) == 0) {
            num_libs++;
        }
    }
    
    if (num_libs == 0) {
        fprintf(stderr, "No libraries loaded!\n");
        return 1;
    }
    
    printf("\nLoaded %d libraries\n\n", num_libs);
    
    // Список тестовых файлов
    const char* test_files[] = {
        "../test_data/GOST_test1.txt",
        "../test_data/GOST_test2.txt",
        "../test_data/random_1k.bin",
        "../test_data/random_1m.bin",
        NULL
    };
    
    // Генерируем тестовые данные если их нет
    printf("Generating test data...\n");
    system("mkdir -p test_data");
    system("./random_generator ../test_data/random_1k.bin 1024");
    system("./random_generator ../test_data/random_1m.bin 1048576");

    
    // Запускаем тесты для каждого файла
    for (int i = 0; test_files[i] != NULL; i++) {
        const char* filename = test_files[i];
        printf("\nTesting with: %s\n", filename);
        
        test_result *results[10] = {NULL}; // Максимум 10 библиотек
        
        // Тестируем каждую реализацию
        for (int lib_idx = 0; lib_idx < num_libs; lib_idx++) {
            hash_implementation* impl = loaded_libs[lib_idx].impl;
            printf("  Testing %s...\n", impl->name);
            
            // Тестируем оба размера хеша
            test_result result_256 = test_implementation(impl, filename, 256);
            test_result result_512 = test_implementation(impl, filename, 512);
            
            // Сохраняем результаты
            if (result_256.status == 0 && result_512.status == 0) {
                results[lib_idx] = malloc(sizeof(test_result));
                *results[lib_idx] = result_256; // Сохраняем 256-битный результат
                memcpy(results[lib_idx]->hash_512, result_512.hash_512, 64);
            }
        }

        // Сравниваем результаты
        for (int j = 1; j < num_libs; j++){
            if (results[0] && results[j]) {
                compare_results(results[0], results[j], 256);
                compare_results(results[0], results[j], 512);
            }
        }
        
        // Очистка
        for (int j = 0; j < num_libs; j++) {
            free(results[j]);
        }
    }


    printf("=== Starting %d random tests ===\n", NUM_OF_RANDOM_TESTS);
    for (int i = 0; i < NUM_OF_RANDOM_TESTS; i++){
        print_progress_bar(i + 1, NUM_OF_RANDOM_TESTS);
        const char *base_command = "./random_generator ../test_data/random_random_size.bin ";

        // Генерация случайного числа в диапазоне 1 байт - 100 Кибибайт
        int random_size = 1 + rand() % (100 * 1024);
        // Буфер для числовой строки
        char random_size_str[10];  // достаточно для 100 * 1024^2 = 104_857_600
        // Преобразование числа в строку
        sprintf(random_size_str, "%d", random_size);

        char command[256];
        // Формируем полную команду конкатенацией
        snprintf(command, sizeof(command), "%s%s >/dev/null", base_command, random_size_str);
        system(command);

        run_compare(loaded_libs, num_libs);      

        system("rm ../test_data/random_random_size.bin");
    }

    // Выгружаем библиотеки
    for (int i = 0; i < num_libs; i++) {
        unload_library(&loaded_libs[i]);
    }
    
    printf("\n=== Comparison complete ===\n");
    return 0;
}