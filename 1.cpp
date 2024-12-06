#include <iostream>
#include <random>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <semaphore.h>
#include <condition_variable>
#include <atomic>

#define NUM_ITERATIONS 500// Количество итераций для потоков

using namespace std;

// Функция для генерации случайного ASCII символа
void generateRandomChar(char& ch) {
    random_device rd; // Генератор случайных чисел
    mt19937 gen(rd()); // Инициализация генератора
    uniform_int_distribution<> dist(32, 126); // Распределение для символов ASCII
    ch = static_cast<char>(dist(gen)); // Генерация символа
}

// Простая реализация семафора
class SimpleSemaphore {
public:
    SimpleSemaphore(int count) : count(count) {
        sem_init(&sem, 0, count); // Инициализация семафора с заданным количеством
    }
    
    void acquire() {
        sem_wait(&sem); // Захват семафора
    }

    void release() {
        sem_post(&sem); // Освобождение семафора
    }

private:
    sem_t sem; // POSIX семафор
    int count; // Количество доступных ресурсов
};

// Упрощенный семафор с блокировкой
class SimpleSemaphoreSlim {
public:
    SimpleSemaphoreSlim(int initialCount, int maxCount) : count(initialCount), maxCount(maxCount) {}

    void acquire() {
        unique_lock<mutex> lock(mtx); // Блокировка мьютекса
        cv.wait(lock, [this]() { // Ожидание, пока счетчик не станет больше 0
            return count > 0;
        });
        --count; // Уменьшение счетчика
    }

    void release() {
        lock_guard<mutex> lock(mtx); // Блокировка мьютекса
        if (count < maxCount) {
            ++count; // Увеличение счетчика
            cv.notify_one(); // Уведомление одного ожидающего потока
        }
    }

private:
    mutex mtx; // Мьютекс для защиты доступа
    condition_variable cv; // Условная переменная для ожидания
    int count, maxCount; // Текущий и максимальный счетчики
};

// Барьер для синхронизации потоков
class SimpleBarrier {
public:
    SimpleBarrier(int numThreads) : totalThreads(numThreads), waitingThreads(0) {}

    void wait() {
        unique_lock<mutex> lock(mtx); // Блокировка мьютекса
        waitingThreads++; // Увеличение счетчика ожидающих потоков
        if (waitingThreads == totalThreads) { // Если все потоки достигли барьера
            waitingThreads = 0; // Сброс счетчика
            cv.notify_all(); // Уведомление всех ожидающих потоков
        } else {
            cv.wait(lock); // Ожидание, пока не будет уведомление
        }
    }

private:
    mutex mtx; // Мьютекс для защиты доступа
    condition_variable cv; // Условная переменная для ожидания
    int totalThreads, waitingThreads; // Общее количество потоков и количество ожидающих
};

// Монитор для синхронизации
class SimpleMonitor {
public:
    SimpleMonitor() : ready(false) {}

    void lock() {
        unique_lock<mutex> lock(mtx); // Блокировка мьютекса
        while (ready) { // Ожидание, если монитор уже занят
            cv.wait(lock); // Ожидание уведомления
        }
        ready = true; // Установка состояния готовности
    }

    void unlock() {
        lock_guard<mutex> lock(mtx); // Блокировка мьютекса
        ready = false; // Освобождение монитора
        cv.notify_one(); // Уведомление одного ожидающего потока
    }

private:
    mutex mtx; // Мьютекс для защиты доступа
    condition_variable cv; // Условная переменная для ожидания
    bool ready; // Флаг занятости монитора
};

// Функция для потоков с мьютексом
void threadWithMutex(char& ch, mutex& mtx, vector<char>& symbols) {
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        generateRandomChar(ch); // Генерация символа
        lock_guard<mutex> lock(mtx); // Блокировка мьютекса
        symbols.push_back(ch); // Добавление символа в вектор
    }
}

// Функция для потоков с семафором
void threadWithSemaphore(char& ch, SimpleSemaphore& sem, vector<char>& symbols) {
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        generateRandomChar(ch); // Генерация символа
        sem.acquire(); // Захват семафора
        symbols.push_back(ch); // Добавление символа в вектор
        sem.release(); // Освобождение семафора
    }
}

// Функция для потоков с упрощенным семафором
void threadWithSemaphoreSlim(char& ch, SimpleSemaphoreSlim& semSlim, vector<char>& symbols) {
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        generateRandomChar(ch); // Генерация символа
        semSlim.acquire(); // Захват семафора
        symbols.push_back(ch); // Добавление символа в вектор
        semSlim.release(); // Освобождение семафора
    }
}

// Функция для потоков с барьером
void threadWithBarrier(char& ch, SimpleBarrier& barrier, vector<char>& symbols) {
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        generateRandomChar(ch); // Генерация символа
        barrier.wait(); // Ожидание достижения барьера
        symbols.push_back(ch); // Добавление символа в вектор
    }
}

// Функция для потоков с монитором
void threadWithMonitor(char& ch, SimpleMonitor& monitor, vector<char>& symbols) {
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        generateRandomChar(ch); // Генерация символа
        monitor.lock(); // Захват монитора
        symbols.push_back(ch); // Добавление символа в вектор
        monitor.unlock(); // Освобождение монитора
    }
}

// Главная функция
int main() {
    system("chcp 65001"); 
    vector<char> symbols; // Вектор для хранения символов
    mutex mtx; // Мьютекс для защиты доступа
    char ch; // Переменная для символа

    // Тестирование мьютекса
    auto start = chrono::high_resolution_clock::now(); // Запуск таймера
    vector<thread> threads; // Вектор потоков
    for (int i = 0; i < 4; i++) {
        threads.emplace_back(threadWithMutex, ref(ch), ref(mtx), ref(symbols)); // Запуск потоков
    }
    for (auto& t : threads) {
        t.join(); // Ожидание завершения потоков
    }
    auto end = chrono::high_resolution_clock::now(); // Окончание таймера
    cout << "Время работы с мьютексом: " << chrono::duration<double>(end - start).count() << " сек\n"; // Вывод времени
    threads.clear(); // Очистка вектора потоков
    symbols.clear(); // Очистка вектора символов

    // Тестирование семафора
    SimpleSemaphore sem(4); // Инициализация семафора
    start = chrono::high_resolution_clock::now(); // Запуск таймера
    for (int i = 0; i < 4; i++) {
        threads.emplace_back(threadWithSemaphore, ref(ch), ref(sem), ref(symbols)); // Запуск потоков
    }
    for (auto& t : threads) {
        t.join(); // Ожидание завершения потоков
    }
    end = chrono::high_resolution_clock::now(); // Окончание таймера
    cout << "Время работы с семафором: " << chrono::duration<double>(end - start).count() << " сек\n"; // Вывод времени
    threads.clear(); // Очистка вектора потоков
    symbols.clear(); // Очистка вектора символов

    // Тестирование упрощенного семафора
    SimpleSemaphoreSlim semSlim(4, 4); // Инициализация упрощенного семафора
    start = chrono::high_resolution_clock::now(); // Запуск таймера
    for (int i = 0; i < 4; i++) {
        threads.emplace_back(threadWithSemaphoreSlim, ref(ch), ref(semSlim), ref(symbols)); // Запуск потоков
    }
    for (auto& t : threads) {
        t.join(); // Ожидание завершения потоков
    }
    end = chrono::high_resolution_clock::now(); // Окончание таймера
    cout << "Время работы с упрощенным семафором: " << chrono::duration<double>(end - start).count() << " сек\n"; // Вывод времени
    threads.clear(); // Очистка вектора потоков
    symbols.clear(); // Очистка вектора символов

    // Тестирование барьера
    SimpleBarrier barrier(4); // Инициализация барьера
    start = chrono::high_resolution_clock::now(); // Запуск таймера
    for (int i = 0; i < 4; i++) {
        threads.emplace_back(threadWithBarrier, ref(ch), ref(barrier), ref(symbols)); // Запуск потоков
    }
    for (auto& t : threads) {
        t.join(); // Ожидание завершения потоков
    }
    end = chrono::high_resolution_clock::now(); // Окончание таймера
    cout << "Время работы с барьером: " << chrono::duration<double>(end - start).count() << " сек\n"; // Вывод времени
    threads.clear(); // Очистка вектора потоков
    symbols.clear(); // Очистка вектора символов

    // Тестирование монитора
    SimpleMonitor monitor; // Инициализация монитора
    start = chrono::high_resolution_clock::now(); // Запуск таймера
    for (int i = 0; i < 4; i++) {
        threads.emplace_back(threadWithMonitor, ref(ch), ref(monitor), ref(symbols)); // Запуск потоков
    }
    for (auto& t : threads) {
        t.join(); // Ожидание завершения потоков
    }
    end = chrono::high_resolution_clock::now(); // Окончание таймера
    cout << "Время работы с монитором: " << chrono::duration<double>(end - start).count() << " сек\n"; // Вывод времени

    return 0; // Завершение программы
}
