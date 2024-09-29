#include "actionrecord.h"
#include <QDebug>
#include "../QtScrcpyCore/include/QtScrcpyCore.h"
#include "QDir"
#include <QClipboard>
#include <QShortcut>

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


void ActionRecord::on_domain_currentTextChanged(const QString &arg1)
{
    ui->subdomain->clear();
    if (arg1 == "Lifestyle") {
        ui->subdomain->addItem("Food-Delivery");
        ui->subdomain->addItem("Shopping");
        ui->subdomain->addItem("Traveling");
    } else if (arg1 == "System") {
        ui->subdomain->addItem("Settings");
        ui->subdomain->addItem("Interaction");
    } else if (arg1 == "Tools") {
        ui->subdomain->addItem("Browser");
        ui->subdomain->addItem("Productivity");
    } else if (arg1 == "Multimedia") {
        ui->subdomain->addItem("Music");
        ui->subdomain->addItem("Video");
    } else if (arg1 == "Communication") {
        ui->subdomain->addItem("Community");
        ui->subdomain->addItem("Email");
        ui->subdomain->addItem("Social-Networking");
        ui->subdomain->addItem("Instant-Messaging");
    }
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
    QString content;
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

