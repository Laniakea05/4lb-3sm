#include <iostream> // Подключаем библиотеку для ввода-вывода
#include <thread>   // Подключаем библиотеку для работы с потоками
#include <mutex>    // Подключаем библиотеку для работы с мьютексами (для синхронизации)
#include <condition_variable> // Подключаем библиотеку для работы с условными переменными
#include <vector>   // Подключаем библиотеку для работы с динамическими массивами (векторами)
#include <chrono>   // Подключаем библиотеку для работы с временными задержками

using namespace std; // Используем пространство имен std для упрощения записи

// Класс Server (Сервер), который управляет вилками
class Server {
private:
    mutex accessMutex; // Мьютекс для синхронизации доступа к вилкам
    condition_variable conditionVar; // Условная переменная для уведомления о доступности вилок
    vector<bool> utensils; // Вектор для отслеживания состояния вилок: true — свободна, false — занята

public:
    // Конструктор, инициализирующий количество вилок
    Server(int totalUtensils) : utensils(totalUtensils, true) {} // Инициализируем вектор вилок, устанавливая все в true (свободны)

    // Метод для запроса разрешения на использование вилок
    void requestUtensils(int leftUtensil, int rightUtensil) {
        unique_lock<mutex> lock(accessMutex); // Блокируем мьютекс для потока
        // Ждем, пока обе вилки станут свободными
        conditionVar.wait(lock, [this, leftUtensil, rightUtensil]() { 
            return utensils[leftUtensil] && utensils[rightUtensil]; // Условие ожидания
        });

        // Забираем вилки
        utensils[leftUtensil] = false; // Устанавливаем вилку слева как занятую
        utensils[rightUtensil] = false; // Устанавливаем вилку справа как занятую
    }

    // Метод для освобождения вилок
    void releaseUtensils(int leftUtensil, int rightUtensil) {
        unique_lock<mutex> lock(accessMutex); // Блокируем мьютекс для потока
        // Освобождаем вилки
        utensils[leftUtensil] = true; // Устанавливаем вилку слева как свободную
        utensils[rightUtensil] = true; // Устанавливаем вилку справа как свободную

        // Уведомляем всех ожидающих о том, что вилки стали доступны
        conditionVar.notify_all(); // Уведомляем все потоки, ожидающие на условной переменной
    }
};

// Класс Thinker (Мыслитель), представляющий философа
class Thinker {
private:
    int thinkerId; // Идентификатор мыслителя
    Server &server; // Ссылка на сервер, который управляет вилками
    std::mutex &leftUtensil; // Мьютекс для левой вилки
    std::mutex &rightUtensil; // Мьютекс для правой вилки

public:
    // Конструктор, инициализирующий мыслителя
    Thinker(int id, Server &server, std::mutex &leftUtensil, std::mutex &rightUtensil)
        : thinkerId(id), server(server), leftUtensil(leftUtensil), rightUtensil(rightUtensil) {}

    // Метод, который мыслитель выполняет для еды и размышлений
    void perform() {
        while (true) { // Бесконечный цикл
            reflect(); // Мыслитель размышляет
            // Запрос разрешения у сервера на использование вилок (левая и правая)
            server.requestUtensils(thinkerId, (thinkerId + 1) % 5); 
            consume(); // Мыслитель ест
            // Освобождение вилок после еды
            server.releaseUtensils(thinkerId, (thinkerId + 1) % 5); 
        }
    }

    // Метод для размышлений мыслителя
    void reflect() {
        std::cout << "Мыслитель " << thinkerId << " размышляет...\n"; // Выводим сообщение о размышлениях
        // Симуляция времени размышлений (от 1 до 2 секунд)
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 + rand() % 1000)); 
    }

    // Метод для еды мыслителя
    void consume() {
        std::lock(leftUtensil, rightUtensil); // Захватываем мьютексы для вилок
        std::lock_guard<std::mutex> leftLock(leftUtensil, std::adopt_lock); // Захватываем левую вилку
        std::lock_guard<std::mutex> rightLock(rightUtensil, std::adopt_lock); // Захватываем правую вилку
        std::cout << "Мыслитель " << thinkerId << " ест.\n"; // Выводим сообщение о еде
        // Симуляция времени еды (от 1 до 2 секунд)
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 + rand() % 1000)); 
        std::cout << "Мыслитель " << thinkerId << " закончил есть.\n"; // Выводим сообщение о завершении еды
    }
};

int main() {
    const int totalThinkers = 5; // Количество мыслителей

    std::vector<std::mutex> utensils(totalThinkers); // Создаем массив мьютексов для вилок
    Server server(totalThinkers); // Создаем сервер, который будет управлять вилками

    std::vector<std::thread> thinkerThreads; // Вектор для потоков мыслителей

    // Создаем мыслителей и соответствующие потоки
    for (int i = 0; i < totalThinkers; ++i) {
        thinkerThreads.emplace_back(
            [i, &server, &utensils]() {
                // Создаем объект мыслителя и запускаем его метод perform() в потоке
                Thinker thinker(i, server, utensils[i], utensils[(i + 1) % totalThinkers]);
                thinker.perform(); 
            }
        );
    } 

    // Ожидаем завершения всех потоков
    for (auto &thread : thinkerThreads) {
        thread.join(); // Ожидаем завершения каждого потока
    }

    return 0; // Завершение программы
}
