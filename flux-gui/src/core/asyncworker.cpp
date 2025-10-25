#include "asyncworker.h"
#include <QThread>
#include <QFile>
#include <QTimer> // Example usage only
#include <chrono>
#include <thread>

AsyncWorker::AsyncWorker(QObject *parent) : QObject(parent) {}
AsyncWorker::~AsyncWorker() {}

void AsyncWorker::process() {
    // Example: Assume we need to process a set of files (simulated with loop here)
    const int total = 100; // Should be determined by actual number of files to process
    for (int i = 0; i < total; ++i) {
        if (m_cancel.load()) {
            emit errorOccurred("Operation cancelled by user");
            return;
        }

        // -------- Put actual time-consuming tasks here (I/O / compression / CPU) --------
        // Example sleep to simulate processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        // ----------------------------------------------------------------------------

        // Emit signal for each processed file
        int percent = static_cast<int>((i + 1) * 100 / total);
        emit progress(percent);
        emit fileProcessed(QString("file_%1.txt").arg(i + 1));
    }

    emit finished();
}
