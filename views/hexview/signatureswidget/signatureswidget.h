#ifndef SIGNATURESWIDGET_H
#define SIGNATURESWIDGET_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include "qhexedit/qhexeditdata.h"
#include "qhexedit/qhexeditdatareader.h"
#include "viewmodels/signaturesmodel/signaturesmodel.h"
#include "signaturesworker.h"

namespace Ui {
class SignaturesWidget;
}

class SignaturesWidget : public QWidget
{
    Q_OBJECT

    public:
        explicit SignaturesWidget(QWidget *parent = 0);
        void scan(QHexEditData* hexeditdata);
        ~SignaturesWidget();

    private slots:
        void onSignaturesWorkerFinished();

    private:
        Ui::SignaturesWidget *ui;
        QHexEditData* _hexeditdata;
        SignaturesWorker _worker;
        SignaturesModel* _signaturesmodel;
};

#endif // SIGNATURESWIDGET_H