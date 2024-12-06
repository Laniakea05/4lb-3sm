#include <iostream>     // Для ввода и вывода
#include <vector>      // Для использования векторов
#include <thread>      // Для работы с потоками
#include <chrono>      // Для работы с временем
#include <mutex>       // Для использования мьютексов
#include <iomanip>     // Для форматирования вывода
#include <ctime>       // Для работы с временем

using namespace std;

// Структура для хранения данных о тренировках
struct Training {
    string date;       // Дата в формате "YYYY-MM-DD"
    string time;       // Время в формате "HH:MM"
    string coachName;  // ФИО тренера
};

// Функция для проверки, является ли дата тренировок днем недели D
bool isTrainingOnDay(const Training& training, int dayOfWeek) {
    // Преобразуем строку даты в структуру tm
    tm tm = {};
    stringstream ss(training.date); // Создаем строковый поток из даты
    ss >> get_time(&tm, "%Y-%m-%d"); // Заполняем структуру tm из строки
    // Проверяем, соответствует ли день недели заданному
    return (tm.tm_wday == dayOfWeek); // Возвращаем true, если день недели совпадает
}

// Функция обработки данных без многопоточности
void processWithoutThreads(const vector<Training>& trainings, int dayOfWeek, vector<Training>& results) {
    for (const auto& training : trainings) { // Проходим по всем тренировкам
        if (isTrainingOnDay(training, dayOfWeek)) { // Проверяем, совпадает ли день недели
            results.push_back(training); // Если совпадает, добавляем в результаты
        }
    }
}

// Функция обработки данных с использованием многопоточности
void processWithThreads(const vector<Training>& trainings, int dayOfWeek, vector<Training>& results, size_t start, size_t end) {
    for (size_t i = start; i < end; ++i) { // Проходим по подмножеству тренировок
        if (isTrainingOnDay(trainings[i], dayOfWeek)) { // Проверяем, совпадает ли день недели
            results.push_back(trainings[i]); // Если совпадает, добавляем в результаты
        }
    }
}

// Функция для выполнения многопоточной обработки
void multiThreadedProcessing(const vector<Training>& trainings, int dayOfWeek, vector<Training>& results, int numThreads) {
    vector<thread> threads; // Вектор для хранения потоков
    size_t trainingsPerThread = trainings.size() / numThreads; // Количество тренировок на поток

    for (int i = 0; i < numThreads; ++i) {
        size_t start = i * trainingsPerThread; // Начало диапазона для потока
        size_t end = (i == numThreads - 1) ? trainings.size() : start + trainingsPerThread; // Конец диапазона
        threads.emplace_back(processWithThreads, ref(trainings), dayOfWeek, ref(results), start, end); // Создаем поток
    }

    for (auto& t : threads) {
        t.join(); // Ожидаем завершения всех потоков
    }
}

int main() {
    // Задаем данные
    int size;          // Количество тренировок
    int numThreads;    // Количество параллельных потоков
    int dayOfWeek;     // 0 - воскресенье, 1 - понедельник, ..., 6 - суббота

    cout << "Введите размер массива данных (количество тренировок): ";
    cin >> size; // Ввод количества тренировок

    cout << "Введите количество параллельных потоков: ";
    cin >> numThreads; // Ввод количества потоков

    cout << "Введите день недели (0 - воскресенье, 1 - понедельник, ..., 6 - суббота): ";
    cin >> dayOfWeek; // Ввод дня недели

    vector<Training> trainings(size); // Вектор для хранения тренировок
    vector<Training> results; // Вектор для хранения результатов

    // Заполнение массива тренировок (например, случайными данными)
    for (int i = 0; i < size; ++i) {
        cout << "Введите дату (YYYY-MM-DD) для тренировки " << (i + 1) << ": ";
        cin >> trainings[i].date; // Ввод даты тренировки
        cout << "Введите время (HH:MM) для тренировки " << (i + 1) << ": ";
        cin >> trainings[i].time; // Ввод времени тренировки
        cin.ignore(); // Игнорируем символ новой строки после ввода времени
        cout << "Введите ФИО тренера для тренировки " << (i + 1) << ": ";
        getline(cin, trainings[i].coachName); // Ввод ФИО тренера с пробелами
    }

    // Обработка без многопоточности
    auto start = chrono::high_resolution_clock::now(); // Запоминаем время начала
    processWithoutThreads(trainings, dayOfWeek, results); // Обработка данных
    auto end = chrono::high_resolution_clock::now(); // Запоминаем время окончания
    double timeWithoutThreads = chrono::duration<double>(end - start).count(); // Вычисляем время обработки

    // Вывод результатов
    cout << "Результаты обработки без многопоточности:\n";
    for (const auto& training : results) {
        cout << training.date << " " << training.time << " " << training.coachName << endl; // Выводим результаты
    }
    
    // Устанавливаем формат вывода времени
    cout << fixed << setprecision(5); // Устанавливаем фиксированное число десятичных знаков
    cout << "Время обработки без многопоточности: " << timeWithoutThreads << " секунд\n"; // Выводим время обработки

    // Очистка результатов для следующей обработки
    results.clear(); // Очищаем вектор результатов

    // Обработка с использованием многопоточности
    start = chrono::high_resolution_clock::now(); // Запоминаем время начала
    multiThreadedProcessing(trainings, dayOfWeek, results, numThreads); // Обработка данных
    end = chrono::high_resolution_clock::now(); // Запоминаем время окончания
    double timeWithThreads = chrono::duration<double>(end - start).count(); // Вычисляем время обработки

    // Вывод результатов
    cout << "Результаты обработки с использованием многопоточности:\n";
    for (const auto& training : results) {
        cout << training.date << " " << training.time << " " << training.coachName << endl; // Выводим результаты
    }
    
    // Устанавливаем формат вывода времени
    cout << fixed << setprecision(5); // Устанавливаем фиксированное число десятичных знаков
    cout << "Время обработки с использованием многопоточности: " << timeWithThreads << " секунд\n"; // Выводим время обработки

    return 0; // Завершаем программу
}
