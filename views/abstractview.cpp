#include "abstractview.h"

AbstractView::AbstractView(QHexEditData* hexeditdata, const QString &viewname, QLabel* labelinfo, QWidget *parent): QWidget(parent), _lblinfo(labelinfo), _hexeditdata(hexeditdata), _viewname(viewname)
{

}

AbstractView::~AbstractView()
{

}

bool AbstractView::canSaveAs() const
{
    return this->canSave();
}

QString AbstractView::saveFilter() const
{
    return "All Files| *.*";
}

const QString &AbstractView::viewName() const
{
    return this->_viewname;
}

void AbstractView::save(const QString &, const QString &)
{

}

void AbstractView::save()
{

}

void AbstractView::updateInfoText(const QString &s)
{
    this->_lblinfo->setText(s);
}
