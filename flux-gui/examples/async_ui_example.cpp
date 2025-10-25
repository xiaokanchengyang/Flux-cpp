// Example usage in UI (e.g., MainWindow)
// This demonstrates how to use AsyncWorker for non-blocking operations

#include "core/asyncworker.h"
#include <QMainWindow>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QThread>

class MainWindow : public QMainWindow {
    Q_OBJECT

private:
    QProgressBar *progressBar;
    QLabel *statusLabel;
    QPushButton *cancelButton;
    QPushButton *startButton;

public slots:
    void startAsyncOperation() {
        // Create worker and thread
        auto *worker = new AsyncWorker();
        auto *thread = new QThread(this);

        worker->moveToThread(thread);

        // Connect signals
        connect(thread, &QThread::started, worker, &AsyncWorker::process);
        connect(worker, &AsyncWorker::finished, thread, &QThread::quit);
        connect(worker, &AsyncWorker::finished, worker, &AsyncWorker::deleteLater);
        connect(thread, &QThread::finished, thread, &QThread::deleteLater);

        // UI -> Worker signals
        connect(worker, &AsyncWorker::progress, this, [this](int percent){
            progressBar->setValue(percent);
        });
        connect(worker, &AsyncWorker::fileProcessed, this, [this](const QString &fname){
            statusLabel->setText(QString("Processing: %1").arg(fname));
        });
        connect(worker, &AsyncWorker::errorOccurred, this, [this](const QString &msg){
            QMessageBox::warning(this, "Task Interrupted", msg);
        });

        // Start thread (e.g., on button click)
        thread->start();

        // Cancel operation example (on cancel button click)
        connect(cancelButton, &QPushButton::clicked, this, [worker](){
            worker->requestCancel();
        });
    }
};

/*
Key Points:

1. Put actual compression/extraction logic in process(), avoiding direct UI operations 
   (UI can only be accessed from main thread).

2. requestCancel() uses atomic boolean to interrupt long loops. For libraries requiring 
   finer-grained cancellation control (e.g., libarchive), prefer library-provided 
   interrupt callbacks or checkpoints.

3. For large file previews (hex or images), use streaming reads and emit 
   fileProcessed/previewReady signals from Worker, then render in UI on main thread 
   to avoid loading entire large files into memory.

4. For parallel processing of multiple files, consider using QThreadPool + QRunnable 
   in main thread, or create multiple workers and thread pools, but watch for 
   IO limitations and lock contention.
*/
