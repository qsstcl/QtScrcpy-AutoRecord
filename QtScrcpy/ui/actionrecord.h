#ifndef ACTIONRECORD_H
#define ACTIONRECORD_H

#include <QWidget>
#include "ui_actionrecord.h"

#include <QVector>
#include <QString>
#include <QMap>

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
    void step();
    void loadTasks();
    bool fakeModeActivated();

private slots:
    void on_startButton_clicked();

    void on_endButton_clicked();

    void on_stepButton_clicked();

    void on_domain_currentTextChanged(const QString &text);

    void on_nextEpsButton_clicked();

    void on_lineEdit_returnPressed();

    void on_checkBox_stateChanged(int state);

private:
    Ui::ActionRecord *ui;
    QVector<QString> curStepActions;
    QVector<QString> curEpsActions;
    QString serial;
    bool isRecording;
    bool isFakeModeActivated;
    QMap<QString, QMap<QString, QVector<QString>>> tasks;
};

#endif // ACTIONRECORD_H
