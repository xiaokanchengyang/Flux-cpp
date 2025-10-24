#include "extract_view.h"
#include <QVBoxLayout>
#include <QLabel>

ExtractView::ExtractView(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* label = new QLabel("Extract View - To be implemented");
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
}

