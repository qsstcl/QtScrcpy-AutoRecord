#ifndef ACTIONRECORD_H
#define ACTIONRECORD_H

#include <QWidget>
#include "ui_actionrecord.h"

#include <QVector>
#include <QString>

namespace Ui
{
    class ActionRecord;
}


class ActionRecord : public QWidget
{
    Q_OBJECT

public:
    explicit ActionRecord(QWidget *parent = nullptr);
    ~ActionRecord();
    static ActionRecord& getInstance();
    void appendAction(const QString& action);
    void setSerial(const QString& serial);
    bool recording();

private slots:
    void on_startButton_clicked();

    void on_endButton_clicked();

    void on_stepButton_clicked();

    void on_domain_currentTextChanged(const QString &arg1);

private:
    Ui::ActionRecord *ui;
    QVector<QString> curStepActions;
    QString serial;
    bool isRecording;
};

#endif // ACTIONRECORD_H
