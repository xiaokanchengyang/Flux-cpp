#pragma once
#include <QObject>
#include <QThread>
#include <atomic>

class AsyncWorker : public QObject {
    Q_OBJECT
public:
    explicit AsyncWorker(QObject *parent = nullptr);
    ~AsyncWorker();

    // Set cancellation flag
    void requestCancel() { m_cancel.store(true); }

public slots:
    // Actual work to be performed (e.g., compression/extraction)
    void process();

signals:
    void progress(int percent);           // 0..100
    void fileProcessed(const QString &file);
    void finished();
    void errorOccurred(const QString &message);

private:
    std::atomic<bool> m_cancel{false};
};
