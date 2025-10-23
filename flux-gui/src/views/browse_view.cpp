#include "browse_view.h"
#include <QVBoxLayout>
#include <QLabel>

BrowseView::BrowseView(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* label = new QLabel("浏览视图 - 待实现");
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
}

