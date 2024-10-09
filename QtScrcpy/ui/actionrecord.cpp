#include "actionrecord.h"
#include <QDebug>
#include "../QtScrcpyCore/include/QtScrcpyCore.h"
#include "QDir"
#include <QClipboard>
#include <QShortcut>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

ActionRecord::ActionRecord(QWidget *parent) : QWidget(parent), ui(new Ui::ActionRecord) {
    ui->setupUi(this);
    ui->comboBox->addItem("OnePlus");
    ui->comboBox->addItem("Huawei");
    ui->comboBox->setCurrentIndex(0);

    ui->domain->addItem("Lifestyle");
    ui->domain->addItem("Tools");
    ui->domain->addItem("Communication");
    ui->domain->addItem("Multimedia");
    ui->domain->addItem("System");
    ui->domain->setCurrentIndex(0);

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

ActionRecord &ActionRecord::getInstance()
{
    static ActionRecord instance;
    return instance;
}

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
    if (!file.open(QIODevice::ReadWrite)) {
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
    if (!logFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        qInfo() << "Open file " << filename << " failed.";
        return;
    }
    QString content = QString("task: %1\n").arg(ui->taskEdit->text());
    for (auto& s : this->curEpsActions) {
        content += s + "\n";
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

