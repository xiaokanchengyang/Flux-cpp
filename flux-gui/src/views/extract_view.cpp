#include "extract_view.h"
#include <QVBoxLayout>
#include <QLabel>

ExtractView::ExtractView(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* label = new QLabel("解压视图 - 待实现");
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
}

