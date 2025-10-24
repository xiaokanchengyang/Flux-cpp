#include "browse_view.h"
#include <QVBoxLayout>
#include <QLabel>

BrowseView::BrowseView(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* label = new QLabel("Browse View - To be implemented");
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
}

