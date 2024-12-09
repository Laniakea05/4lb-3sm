#include <iostream>     // Для ввода и вывода
#include <vector>      // Для использования векторов
#include <thread>      // Для работы с потоками
#include <chrono>      // Для работы с временем
#include <iomanip>     // Для форматирования вывода
#include <ctime>       // Для работы с временем
#include <sstream>     // Для работы со строками
#include <random>      // Для генерации случайных чисел

using namespace std;

// Структура для хранения данных о тренировках
struct Training {
    string date;       // Дата в формате "YYYY-MM-DD"
    string time;       // Время в формате "HH:MM"
    string coachName;  // ФИО тренера
};

// Функция для проверки, является ли дата тренировок днем недели D
bool isTrainingOnDay(const Training& training, int dayOfWeek) {
    tm tm = {};
    stringstream ss(training.date);
    ss >> get_time(&tm, "%Y-%m-%d");
    return (tm.tm_wday == dayOfWeek);
}

// Функция обработки данных с использованием многопоточности
void processWithThreads(const vector<Training>& trainings, int dayOfWeek, vector<Training>& results, size_t start, size_t end) {
    for (size_t i = start; i < end; ++i) {
        if (isTrainingOnDay(trainings[i], dayOfWeek)) {
            results.push_back(trainings[i]);
        }
    }
}

// Функция для выполнения многопоточной обработки
void multiThreadedProcessing(const vector<Training>& trainings, int dayOfWeek, vector<Training>& results, int numThreads) {
    vector<thread> threads;
    size_t trainingsPerThread = trainings.size() / numThreads;

    for (int i = 0; i < numThreads; ++i) {
        size_t start = i * trainingsPerThread;
        size_t end = (i == numThreads - 1) ? trainings.size() : start + trainingsPerThread;
        threads.emplace_back(processWithThreads, ref(trainings), dayOfWeek, ref(results), start, end);
    }

    for (auto& t : threads) {
        t.join();
    }
}

// Функция для генерации случайных данных о тренировках
void generateRandomTrainings(vector<Training>& trainings, int size, const string& startDate, const string& endDate) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, 23);
    uniform_int_distribution<> disMin(0, 59);

    tm startTm = {};
    stringstream ssStart(startDate);
    ssStart >> get_time(&startTm, "%Y-%m-%d");
    time_t startEpoch = mktime(&startTm);

    tm endTm = {};
    stringstream ssEnd(endDate);
    ssEnd >> get_time(&endTm, "%Y-%m-%d");
    time_t endEpoch = mktime(&endTm);

    vector<string> coaches = {"Иванов И.И.", "Петров П.П.", "Сидоров С.С.", "Кузнецов А.А.", "Смирнов В.В."};

    for (int i = 0; i < size; ++i) {
        time_t randomTime = startEpoch + rand() % (endEpoch - startEpoch + 1);
        tm* randomTm = localtime(&randomTime);

        string date = to_string(randomTm->tm_year + 1900) + "-" +
                      to_string(randomTm->tm_mon + 1) + "-" +
                      to_string(randomTm->tm_mday);

        string time = to_string(dis(gen)) + ":" + (disMin(gen) < 10 ? "0" : "") + to_string(disMin(gen));

        string coachName = coaches[rand() % coaches.size()];

        trainings.push_back({date, time, coachName});
    }
}

int main() {
    int size;          // Количество тренировок
    int numThreads;    // Количество параллельных потоков
    int dayOfWeek;     // 0 - воскресенье, 1 - понедельник, ..., 6 - суббота
    string startDate;  // Начальная дата
    string endDate;    // Конечная дата

    cout << "Введите количество тренировок: ";
    cin >> size;

    cout << "Введите количество параллельных потоков: ";
    cin >> numThreads;

    cout << "Введите день недели (0 - воскресенье, 1 - понедельник, ..., 6 - суббота): ";
    cin >> dayOfWeek;

    cout << "Введите начальную дату (YYYY-MM-DD): ";
    cin >> startDate;

    cout << "Введите конечную дату (YYYY-MM-DD): ";
    cin >> endDate;

    vector<Training> trainings;
    vector<Training> results;

    // Генерация случайных тренировок
    generateRandomTrainings(trainings, size, startDate, endDate);

    // Обработка без использования многопоточности
    auto startSingle = chrono::high_resolution_clock::now();
    for (const auto& training : trainings) {
        if (isTrainingOnDay(training, dayOfWeek)) {
            results.push_back(training);
        }
    }
    auto endSingle = chrono::high_resolution_clock::now();
    double timeWithoutThreads = chrono::duration<double>(endSingle - startSingle).count();

    // Вывод результатов без многопоточности
    cout << "Результаты обработки без использования многопоточности:\n";
    for (const auto& training : results) {
        cout << training.date << " " << training.time << " " << training.coachName << endl;
    }

    // Очищаем результаты для многопоточной обработки
    results.clear();

    // Обработка с использованием многопоточности
    auto startMulti = chrono::high_resolution_clock::now();
    multiThreadedProcessing(trainings, dayOfWeek, results, numThreads);
    auto endMulti = chrono::high_resolution_clock::now();
    double timeWithThreads = chrono::duration<double>(endMulti - startMulti).count();

    // Вывод результатов с многопоточностью
    cout << "Результаты обработки с использованием многопоточности:\n";
    for (const auto& training : results) {
        cout << training.date << " " << training.time << " " << training.coachName << endl;
    }

    // Устанавливаем формат вывода времени
    cout << fixed << setprecision(5);
    cout << "Время обработки без использования многопоточности: " << timeWithoutThreads << " секунд\n";
    cout << "Время обработки с использованием многопоточности: " << timeWithThreads << " секунд\n";

    return 0;
}
