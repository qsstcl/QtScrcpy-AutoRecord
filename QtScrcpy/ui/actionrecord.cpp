#include "actionrecord.h"
#include <QDebug>
#include "../QtScrcpyCore/include/QtScrcpyCore.h"
#include "QDir"
#include <QClipboard>
#include <QShortcut>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QThread>
#include <QProcess>
ActionRecord::ActionRecord(QWidget *parent) : QWidget(parent), ui(new Ui::ActionRecord) {
    ui->setupUi(this);
    ui->comboBox->addItem("OnePlus");
    ui->comboBox->addItem("Huawei");
    ui->comboBox->addItem("Samsung");
    ui->comboBox->setCurrentIndex(0);

    ui->launchAppBox->addItem("xhs");
    ui->launchAppBox->addItem("meituan");
    ui->domain->addItem("Multi-app");
    ui->domain->addItem("Lifestyle");
    ui->domain->addItem("Tools");
    ui->domain->addItem("Communication");
    ui->domain->addItem("Multimedia");
    ui->domain->addItem("System");
    ui->domain->setCurrentIndex(0);
    QStringList difficulties = {"1", "2", "3", "4","5"};
    ui->DifficultyBox->addItems(difficulties);
    // launch app logic

    ui->episodeSpin->setMinimum(1);
    ui->stepSpin->setMinimum(1);
    ui->episodeSpin->setValue(1);
    ui->stepSpin->setValue(1);

    this->isRecording = false;
    this->isFakeModeActivated = false;

    auto shortcut = new QShortcut(QKeySequence("Ctrl+d"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        this->step();
    });
}

ActionRecord::~ActionRecord()
{
    delete ui;
}

// ActionRecord &ActionRecord::getInstance()
// {
//     static ActionRecord instance;
//     return instance;
// }

void ActionRecord::appendAction(const QString &action)
{
    this->curStepActions.append(action);
}

void ActionRecord::on_startButton_clicked()
{
    ui->recordingLabel->setText("Recording...");
    this->curEpsActions.clear();
    this->curStepActions.clear();
    this->isRecording = true;
    auto device = qsc::IDeviceManage::getInstance().getDevice(this->serial);
    if (!device) {
        return;
    }
    QString filename = QString("%1/%2/%3/%4/%5.jpg").arg(ui->comboBox->currentText()).arg(ui->domain->currentText()).arg(ui->subdomain->currentText()).arg(ui->episodeSpin->text()).arg(ui->stepSpin->text());
    device->screenshotWithFilename(filename);

    // const QString& recordRootPath = device->getDeviceParams().recordPath;
    // QDir dir(recordRootPath);
    // QString absolutePath = dir.absoluteFilePath(filename.replace(".jpg", ".xml"));
    // dumpXml(absolutePath);
}


void ActionRecord::on_endButton_clicked()
{
    ui->recordingLabel->setText("Recording Stopped");
    this->curEpsActions.clear();
    this->curStepActions.clear();
    this->isRecording = false;
}

void ActionRecord::setSerial(const QString &serial)
{
    this->serial = serial;
}

bool ActionRecord::recording()
{
    return this->isRecording;
}

void ActionRecord::step()
{
    on_stepButton_clicked();
}

void ActionRecord::loadTasks()
{
    this->tasks.clear();
    this->tasks.insert("Lifestyle", {{"Food-Delivery", {}}, {"Shopping", {}}, {"Traveling", {}}});
    this->tasks.insert("System", {{"Settings", {}}, {"Interaction", {}}});
    this->tasks.insert("Tools", {{"Browser", {}}, {"Productivity", {}}});
    this->tasks.insert("Multimedia", {{"Music", {}}, {"Video", {}}});
    this->tasks.insert("Communication", {{"Community", {}}, {"Email", {}}, {"Social-Networking", {}}, {"Instant-Messaging", {}}});

    auto device = qsc::IDeviceManage::getInstance().getDevice(this->serial);
    if (!device) {
        return;
    }

    const QString& recordRootPath = device->getDeviceParams().recordPath;
    QString filename = QString("tasks.json");
    QDir dir(recordRootPath);
    QString absolutePath = dir.absoluteFilePath(filename);
    QFile file(absolutePath);
    if (!file.open(QIODevice::Append)) {
        qInfo() << "Open file " << filename << " failed.";
        return;
    }

    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &jsonError);
    file.close();

    if (jsonError.error != QJsonParseError::NoError && !doc.isNull()) {
        qInfo() << "Parse json file " << filename << " failed.";
        return;
    }

    QJsonObject rootObj = doc.object();
    for (auto it = this->tasks.begin(); it != this->tasks.end(); ++it) {
        QJsonValue subdomainValue = rootObj.value(it.key());
        if (!subdomainValue.isObject()) {
            qInfo() << "Parse json file " << filename << " failed at key \"" << it.key() << "\".";
            return;
        }
        QJsonObject subdomainObject =subdomainValue.toObject();
        auto& subdomainMap = it.value();
        for (auto it2 = subdomainMap.begin(); it2 != subdomainMap.end(); ++it2) {
            QJsonValue taskArrayValue = subdomainObject.value(it2.key());
            if (!taskArrayValue.isArray()) {
                qInfo() << "Parse json file " << filename << " failed at key \"" << it2.key() << "\".";
                return;
            }
            QJsonArray taskArray = taskArrayValue.toArray();
            for (int i = 0; i < taskArray.size(); ++i) {
                QJsonValue taskValue = taskArray.at(i);
                if (!taskValue.isString()) {
                    qInfo() << "Parse json file " << filename << " failed at key \"" << it2.key() << "\".";
                    return;
                }
                it2.value().append(taskValue.toString());
            }
        }
    }
}

bool ActionRecord::fakeModeActivated()
{
    return this->isFakeModeActivated;
}

void ActionRecord::bufferedPress(int x, int y)
{
    this->bufferedPressTime = QDateTime::currentMSecsSinceEpoch();
    this->bufferedPressPos = qMakePair(x, y);
}

void ActionRecord::bufferedRelease(int x, int y)
{
    qint64 ts = QDateTime::currentMSecsSinceEpoch();
    if (this->bufferedPressTime < 0 || ts < this->bufferedPressTime)
        return;
    qint64 diff = ts - this->bufferedPressTime;
    int xDist = x - bufferedPressPos.first, yDist = y - bufferedPressPos.second;
    int dist = xDist * xDist + yDist * yDist;
    if (diff >= 700 && dist <= 100) {
        this->appendAction(QString("LONGCLICK [%1, %2]").arg(x).arg(y));
    } else {
        this->appendAction(QString("PRESS [%1, %2]").arg(bufferedPressPos.first).arg(bufferedPressPos.second));
        this->appendAction(QString("RELEASE [%1, %2]").arg(x).arg(y));
    }
    this->bufferedPressTime = -1;
}

void ActionRecord::setAdbProcess(qsc::AdbProcess *adb)
{
    this->adb = adb;
}


void ActionRecord::on_stepButton_clicked()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(this->serial);
    if (!device) {
        return;
    }

    QString trans = QString("%1 --> %2:").arg(QString::number(ui->stepSpin->value())).arg(QString::number(ui->stepSpin->value() + 1));
    qInfo() << trans;
    this->curEpsActions.append(trans);
    for (auto& action : this->curStepActions) {
        qInfo() << action;
        this->curEpsActions.append(action);
    }
    this->curStepActions.clear();

    ui->stepSpin->stepBy(1);

    QString filename = QString("%1/%2/%3/%4/%5.jpg").arg(ui->comboBox->currentText()).arg(ui->domain->currentText()).arg(ui->subdomain->currentText()).arg(ui->episodeSpin->text()).arg(ui->stepSpin->text());

    device->screenshotWithFilename(filename);

    // const QString& recordRootPath = device->getDeviceParams().recordPath;
    // QDir dir(recordRootPath);
    // QString absolutePath = dir.absoluteFilePath(filename.replace(".jpg", ".xml"));
    // dumpXml(absolutePath);
}

void ActionRecord::on_domain_currentTextChanged(const QString &text)
{
    // if (!this->tasks.contains(arg1))
    //     return;
    ui->subdomain->clear();
    if (text == "Lifestyle") {
        ui->subdomain->addItem("Food-Delivery");
        ui->subdomain->addItem("Shopping");
        ui->subdomain->addItem("Traveling");
    } else if (text == "System") {
        ui->subdomain->addItem("Settings");
        ui->subdomain->addItem("Interaction");
    } else if (text == "Tools") {
        ui->subdomain->addItem("Browser");
        ui->subdomain->addItem("Productivity");
    } else if (text == "Multimedia") {
        ui->subdomain->addItem("Music");
        ui->subdomain->addItem("Video");
    } else if (text == "Communication") {
        ui->subdomain->addItem("Community");
        ui->subdomain->addItem("Email");
        ui->subdomain->addItem("Social-Networking");
        ui->subdomain->addItem("Instant-Messaging");
    } else if (text == "Multi-app"){
        ui->subdomain->addItem("maiyao");
    }
    // auto& map = this->tasks[arg1];
    // for (auto it = map.keyBegin(); it != map.keyEnd(); it++) {
    //     ui->subdomain->addItem(*it);
    // }
}


void ActionRecord::on_nextEpsButton_clicked()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(this->serial);
    if (!device) {
        return;
    }
    const QString& recordRootPath = device->getDeviceParams().recordPath;
    QString filename = QString("%1/%2/%3/%4/actions.log").arg(ui->comboBox->currentText()).arg(ui->domain->currentText()).arg(ui->subdomain->currentText()).arg(ui->episodeSpin->text());
    QDir dir(recordRootPath);

    QString absolutePath = dir.absoluteFilePath(filename);
    QFile logFile(absolutePath);
    if (!logFile.open(QIODevice::Append | QIODevice::Text)) {
        qInfo() << "Open file " << filename << " failed.";
        return;
    }
    QString content = QString("task: %1\n").arg(ui->taskEdit->text());
    for (auto& s : this->curEpsActions) {
        content += s + '\n';
    }
    logFile.write(content.toStdString().c_str());
    logFile.close();

    ui->stepSpin->setValue(1);
    ui->episodeSpin->stepBy(1);
    on_endButton_clicked();
    qInfo() << "action log saved to " << absolutePath;
}


void ActionRecord::on_lineEdit_returnPressed()
{
    if (!recording())
        return;
    auto device = qsc::IDeviceManage::getInstance().getDevice(this->serial);
    if (!device) {
        return;
    }
    QString input = ui->lineEdit->text();
    QClipboard *board = QApplication::clipboard();
    board->setText(input);
    emit device->setDeviceClipboard();
    board->clear();
    // device->postTextInput(input);
    appendAction(QString("INPUT %1").arg(input));
    ui->lineEdit->clear();
}



void ActionRecord::on_checkBox_stateChanged(int state)
{
    qInfo() << state;
    if (state == Qt::Checked) {
        this->isFakeModeActivated = true;
    } else if (state == Qt::Unchecked) {
        this->isFakeModeActivated = false;
    }
}

void ActionRecord::dumpXml(const QString &absPath)
{
    qDebug() << "try dump xml to " << absPath;
    QStringList adbArgs;
    adbArgs << "shell" << "uiautomator" << "dump";
    this->adb->execute(this->serial, adbArgs);
    if (!this->adb->waitForFinished())
        qDebug() << "dump xml timeout";
    adbArgs.clear();
    adbArgs << "pull" << "/sdcard/window_dump.xml" << absPath;
    this->adb->execute(this->serial, adbArgs);
    if (!this->adb->waitForFinished())
        qDebug() << "pull xml timeout";
    qDebug() << "xml dumped to " << absPath;
}

void ActionRecord::on_launchButton_clicked(){
    QString appName = ui->launchAppBox->currentText();

    QMap<QString, QString> appMap = {
        {"xhs", "com.xingin.xhs/com.xingin.xhs.index.v2.IndexActivityV2"},
        {"meituan", "com.sankuai.meituan/com.meituan.android.pt.homepage.activity.MainActivity"}
    };

    if (appMap.contains(appName)) {
        QString command = "adb";
        QStringList arguments = {"shell", "am", "start", "-n", appMap[appName]};

        if (!QProcess::startDetached(command, arguments)) {
            qWarning() << "Failed to start the command.";
        } else {
            qInfo() << "Command executed successfully.";
            QString content = QString("LAUNCH %1").arg(appName);
            appendAction(content);
        }
    } else {
        qInfo() << "No action defined for app name:" << appName;
    }
}

void ActionRecord::on_summaryButton_clicked(){

    appendAction(QString("SUMMARY"));
}

void ActionRecord::on_setDifficultyButton_clicked(){

    auto device = qsc::IDeviceManage::getInstance().getDevice(this->serial);
    if (!device) {
        return;
    }
    const QString& recordRootPath = device->getDeviceParams().recordPath;
    QString filename = QString("%1/%2/%3/%4/actions.log").arg(ui->comboBox->currentText()).arg(ui->domain->currentText()).arg(ui->subdomain->currentText()).arg(ui->episodeSpin->text());
    QDir dir(recordRootPath);

    QString absolutePath = dir.absoluteFilePath(filename);
    QFile logFile(absolutePath);
    if (!logFile.open(QIODevice::Append | QIODevice::Text)) {
        qInfo() << "Open file " << filename << " failed.";
        return;
    }
    QString content = QString("DIFFICULTY %1\n").arg(ui->DifficultyBox->currentText());
    logFile.write(content.toStdString().c_str());
    logFile.close();
}
