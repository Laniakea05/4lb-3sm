#include <iostream> // Библиотека для ввода-вывода
#include <random>   // Библиотека для генерации случайных чисел
#include <mutex>    // Библиотека для мьютексов
#include <thread>   // Библиотека для работы с потоками
#include <vector>   // Библиотека для работы с векторами
#include <chrono>   // Библиотека для работы с временем
#include <semaphore.h> // Библиотека для работы с POSIX семафорами
#include <condition_variable> // Библиотека для условных переменных
#include <atomic>   // Библиотека для атомарных операций

#define N 500 // Константа, определяющая количество итераций для потоков

using namespace std;

// Функция для генерации случайного символа ASCII
void randomSymbols(char& symbol) { 
    random_device rd; // Использует аппаратный генератор случайных чисел
    mt19937 gen(rd()); // Инициализация генератора случайных чисел
    uniform_int_distribution<> dis(32, 126); // Определение диапазона ASCII символов
    symbol = static_cast<char>(dis(gen)); // Генерация случайного символа
}

// Класс для реализации семафора
class Semaphore { 
public:
    // Конструктор, инициализирующий семафор с начальным значением
    Semaphore(int initialCount) : iCount(initialCount) {
        sem_init(&sem, 0, initialCount); // Инициализация POSIX семафора
    }
    
    // Метод для захвата ресурса
    void acquire() { 
        sem_wait(&sem); // Уменьшает счётчик семафора
    }

    // Метод для освобождения ресурса
    void release() { 
        sem_post(&sem); // Увеличивает счётчик семафора
    }

private:
    sem_t sem;  // Объект семафора
    int iCount; // Начальное значение семафора
};

// Класс для реализации упрощенного семафора
class SemaphoreSlim { 
public:
    // Конструктор
    SemaphoreSlim(int initialCount, int maxCount) : iCount(initialCount), mCount(maxCount) {}

    // Метод для захвата семафора
    void acquire() { 
        unique_lock<mutex> lock(mtx); // Защита доступа к iCount
        cv.wait(lock, [this]() { // Ожидание, пока счётчик не станет > 0
            return iCount > 0;
        });
        --iCount; // Уменьшаем счётчик, если семафор захвачен потоком
    }

    // Метод для освобождения семафора
    void release() { 
        lock_guard<mutex> lock(mtx); // Защита доступа к счётчику
        if (iCount < mCount) {
            ++iCount; // Увеличиваем счётчик
            cv.notify_one(); // Уведомляет один из ожидающих потоков
        }
    }

private:
    mutex mtx; // Мьютекс для защиты доступа
    condition_variable cv; // Условная переменная для ожидания
    int iCount, mCount; // Начальное и максимальное значение счётчиков семафора
};

// Класс для реализации барьера
class Barrier { 
public:
    // Конструктор
    Barrier(int count) : initialCount(count), maxCount(count), generationCount(0) {}

    // Метод для ожидания достижения барьера всеми потоками
    void wait() { 
        unique_lock<mutex> lock(mtx); // Защита доступа переменных

        int initialGen = generationCount; // Запоминаем текущее поколение
        if (--initialCount == 0) { // Если все потоки достигли барьера
            generationCount++; // Увеличиваем счётчик поколений
            initialCount = maxCount; // Сбрасываем счётчик
            cv.notify_all(); // Уведомляем все ожидающие потоки
        }
        else {
            cv.wait(lock, [this, initialGen] { // Ожидаем, пока поколение не изменится
                return initialGen != generationCount;
            });
        }
    }

private:
    mutex mtx; // Мьютекс для защиты доступа
    condition_variable cv; // Условная переменная
    int initialCount, maxCount, generationCount; // Счётчики для управления барьером
};

// Класс для реализации монитора
class Monitor { 
public:
    Monitor() : isReady(false) {} // Инициализация состояния

    // Метод для захвата монитора
    void locker() { 
        unique_lock<mutex> lock(mtx); // Защита доступа к переменным

        while(isReady) { // Блокировка, если монитор уже захвачен
            cv.wait(lock); // Ожидание освобождения монитора
        }
        isReady = true; // Устанавливаем состояние захваченности
    }

    // Метод для освобождения монитора
    void unlocker() { 
        lock_guard<mutex> lock(mtx); // Защита доступа
        isReady = false; // Освобождение монитора
        cv.notify_one(); // Уведомляет один из ожидающих потоков
    }

private:
    mutex mtx; // Мьютекс для защиты доступа
    condition_variable cv; // Условная переменная
    bool isReady; // Состояние захваченности монитора
};

// Функция для работы с мьютексом
void threadMutex(char& symbol, mutex& mtx, vector<char>& allSymbols) { 
    for (int i = 0; i < N; i++) { 
        randomSymbols(symbol); // Генерация символа
        lock_guard<mutex> lock(mtx); // Захват мьютекса
        allSymbols.push_back(symbol); // Добавление символа в вектор
    }
}

// Функция для работы с семафором
void threadSemaphore(char& symbol, Semaphore& sem, vector<char>& allSymbols) { 
    for (int i = 0; i < N; i++) {
        randomSymbols(symbol); // Генерация символа
        sem.acquire(); // Захват семафора
        allSymbols.push_back(symbol); // Добавление символа в вектор
        sem.release(); // Освобождение семафора
    }
}

// Функция для работы с упрощенным семафором
void threadSemaphoreSlim(char& symbol, SemaphoreSlim& semSlim, vector<char>& allSymbols) { 
    for (int i = 0; i < N; i++) {
        randomSymbols(symbol); // Генерация символа
        semSlim.acquire(); // Захват семафора
        allSymbols.push_back(symbol); // Добавление символа в вектор
        semSlim.release(); // Освобождение семафора
    }
}

// Функция для работы с барьером
void threadBarrier(char& symbol, Barrier& barrier, vector<char>& allSymbols) { 
    for (int i = 0; i < N; i++) {
        randomSymbols(symbol); // Генерация символа
        barrier.wait(); // Ожидание достижения барьера
        allSymbols.push_back(symbol); // Добавление символа в вектор
    }
}

// Функция для работы с монитором
void threadMonitor(char& symbol, Monitor& monitor, vector<char>& allSymbols) { 
    for (int i = 0; i < N; i++) {
        randomSymbols(symbol); // Генерация символа
        monitor.locker(); // Захват монитора
        allSymbols.push_back(symbol); // Добавление символа в вектор
        monitor.unlocker(); // Освобождение монитора
    }
}

// Функция для работы со спинлоком
void threadSpinLock(char& symbol, atomic_flag& spinLock, vector<char>& allSymbols) { 
    for (int i = 0; i < N; i++) {
        randomSymbols(symbol); // Генерация символа
        while (spinLock.test_and_set(memory_order_acquire)) {} // Ожидание, пока флаг не будет сброшен
        allSymbols.push_back(symbol); // Добавление символа в вектор
        spinLock.clear(memory_order_release); // Сброс флага
    }
}

// Функция для работы с спин-ожиданием
void threadSpinWait(char& symbol, atomic_flag& spinLock, vector<char>& allSymbols) { 
    for (int i = 0; i < N; i++) {
        randomSymbols(symbol); // Генерация символа
        while (spinLock.test_and_set(memory_order_acquire)) { // Ожидание, пока флаг не будет сброшен
            this_thread::yield(); // Передача управления другим потокам
        }
        allSymbols.push_back(symbol); // Добавление символа в вектор
        spinLock.clear(memory_order_release); // Сброс флага
    }
}

// Основная функция
int main() {
    vector<char> allSymbols; // Вектор для хранения сгенерированных символов
    mutex mtx; // Мьютекс для защиты доступа к вектору
    char symbol; // Переменная для хранения символа
    
    // Тестирование мьютекса
    auto start = chrono::high_resolution_clock::now(); // Запуск таймера
    vector<thread> threads; // Вектор потоков
    for (int i = 0; i < 4; i++) { // Запуск 4 потоков
        threads.emplace_back(threadMutex, ref(symbol), ref(mtx), ref(allSymbols)); // Создание потоков
    }
    for (auto& t : threads) {
        t.join(); // Ожидание завершения потоков
    }
    auto end = chrono::high_resolution_clock::now(); // Завершение таймера
    chrono::duration<double> elapsed = end - start; // Вычисление времени выполнения
    cout << "Mutex time: " << elapsed.count() << " seconds\n"; // Вывод времени выполнения
    threads.clear(); // Очистка вектора потоков
    allSymbols.clear(); // Очистка вектора символов

    // Тестирование семафора
    Semaphore sem(4); // Инициализация семафора с максимальным значением 4
    start = chrono::high_resolution_clock::now(); // Запуск таймера
    for (int i = 0; i < 4; i++) {
        threads.emplace_back(threadSemaphore, ref(symbol), ref(sem), ref(allSymbols)); // Создание потоков
    }
    for (auto& t : threads) {
        t.join(); // Ожидание завершения потоков
    }
    end = chrono::high_resolution_clock::now(); // Завершение таймера
    elapsed = end - start; // Вычисление времени выполнения
    cout << "Semaphore time: " << elapsed.count() << " seconds\n"; // Вывод времени выполнения
    threads.clear(); // Очистка вектора потоков
    allSymbols.clear(); // Очистка вектора символов

    // Тестирование упрощенного семафора
    SemaphoreSlim semSlim(4, 4); // Инициализация упрощенного семафора
    start = chrono::high_resolution_clock::now(); // Запуск таймера
    for (int i = 0; i < 4; i++) {
        threads.emplace_back(threadSemaphoreSlim, ref(symbol), ref(semSlim), ref(allSymbols)); // Создание потоков
    }
    for (auto& t : threads) {
        t.join(); // Ожидание завершения потоков
    }
    end = chrono::high_resolution_clock::now(); // Завершение таймера
    elapsed = end - start; // Вычисление времени выполнения
    cout << "SemaphoreSlim time: " << elapsed.count() << " seconds\n"; // Вывод времени выполнения
    threads.clear(); // Очистка вектора потоков
    allSymbols.clear(); // Очистка вектора символов

    // Тестирование барьера
    Barrier barrier(4); // Инициализация барьера для 4 потоков
    start = chrono::high_resolution_clock::now(); // Запуск таймера
    for (int i = 0; i < 4; i++) {
        threads.emplace_back(threadBarrier, ref(symbol), ref(barrier), ref(allSymbols)); // Создание потоков
    }
    for (auto& t : threads) {
        t.join(); // Ожидание завершения потоков
    }
    end = chrono::high_resolution_clock::now(); // Завершение таймера
    elapsed = end - start; // Вычисление времени выполнения
    cout << "Barrier time: " << elapsed.count() << " seconds\n"; // Вывод времени выполнения
    threads.clear(); // Очистка вектора потоков
    allSymbols.clear(); // Очистка вектора символов

    // Тестирование спинлока
    atomic_flag spinLock = ATOMIC_FLAG_INIT; // Инициализация спинлока
    start = chrono::high_resolution_clock::now(); // Запуск таймера
    for (int i = 0; i < 4; i++) {
        threads.emplace_back(threadSpinLock, ref(symbol), ref(spinLock), ref(allSymbols)); // Создание потоков
    }
    for (auto& t : threads) {
        t.join(); // Ожидание завершения потоков
    }
    end = chrono::high_resolution_clock::now(); // Завершение таймера
    elapsed = end - start; // Вычисление времени выполнения
    cout << "SpinLock time: " << elapsed.count() << " seconds\n"; // Вывод времени выполнения
    threads.clear(); // Очистка вектора потоков
    allSymbols.clear(); // Очистка вектора символов

    // Тестирование спин-ожидания
    start = chrono::high_resolution_clock::now(); // Запуск таймера
    for (int i = 0; i < 4; i++) {
        threads.emplace_back(threadSpinWait, ref(symbol), ref(spinLock), ref(allSymbols)); // Создание потоков
    }
    for (auto& t : threads) {
        t.join(); // Ожидание завершения потоков
    }
    end = chrono::high_resolution_clock::now(); // Завершение таймера
    elapsed = end - start; // Вычисление времени выполнения
    cout << "SpinWait time: " << elapsed.count() << " seconds\n"; // Вывод времени выполнения
    threads.clear(); // Очистка вектора потоков
    allSymbols.clear(); // Очистка вектора символов

    // Тестирование монитора
    Monitor monitor; // Инициализация монитора
    start = chrono::high_resolution_clock::now(); // Запуск таймера
    for (int i = 0; i < 4; i++) {
        threads.emplace_back(threadMonitor, ref(symbol), ref(monitor), ref(allSymbols)); // Создание потоков
    }
    for (auto& t : threads) {
        t.join(); // Ожидание завершения потоков
    }
    end = chrono::high_resolution_clock::now(); // Завершение таймера
    elapsed = end - start; // Вычисление времени выполнения
    cout << "Monitor time: " << elapsed.count() << " seconds\n"; // Вывод времени выполнения

    return 0; // Завершение программы
}
