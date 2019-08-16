#include "controls.h"
#include "scoreboard.h"
#include "ui_controls.h"
#include <map>
#include <QDebug>
#include <QTime>
#include <QtWidgets>
#include <QDir>
#include <QStringList>
#include <QFileDialog>

Controls::Controls(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Controls)
{
    ui->setupUi(this);
    setWindowTitle("Openroll - Controls - " + Controls::s_VERSION);
    /* Create Scoreboard as a child widget, but pass flags to make it an
     * independent window, otherwise it will try to draw into the parent
     * window.
     */
    Scoreboard *board = new Scoreboard(this, Qt::WindowFlags(Qt::Window));
    board->setWindowTitle("Openroll - Scoreboard - " + Controls::s_VERSION);
    board->show();

    std::map<int, int> divisionIdxToTimeMap;
    divisionIdxToTimeMap[0] = 120;
    divisionIdxToTimeMap[1] = 120;
    divisionIdxToTimeMap[2] = 120;
    divisionIdxToTimeMap[3] = 180;
    divisionIdxToTimeMap[4] = 180;
    divisionIdxToTimeMap[5] = 180;
    divisionIdxToTimeMap[6] = 240;
    divisionIdxToTimeMap[7] = 240;
    divisionIdxToTimeMap[8] = 240;
    divisionIdxToTimeMap[9] = 240;
    divisionIdxToTimeMap[10] = 240;
    divisionIdxToTimeMap[11] = 240;
    divisionIdxToTimeMap[12] = 300;
    divisionIdxToTimeMap[13] = 300;
    divisionIdxToTimeMap[14] = 300;
    divisionIdxToTimeMap[15] = 360;
    divisionIdxToTimeMap[16] = 420;
    divisionIdxToTimeMap[17] = 480;
    divisionIdxToTimeMap[18] = 600;
    divisionIdxToTimeMap[19] = 300;
    divisionIdxToTimeMap[20] = 300;
    divisionIdxToTimeMap[21] = 360;
    divisionIdxToTimeMap[22] = 360;
    divisionIdxToTimeMap[23] = 360;
    divisionIdxToTimeMap[24] = 300;
    divisionIdxToTimeMap[25] = 300;
    divisionIdxToTimeMap[26] = 300;
    divisionIdxToTimeMap[27] = 300;
    divisionIdxToTimeMap[28] = 300;

    qDebug() << "totalTime is: " << totalTime;
    qDebug() << "clockMinutes default is: " << clockMinutes;
    qDebug() << "clockSeconds default is: " << clockSeconds;

    timer = new QTimer(this);
    m_player = new QMediaPlayer(this);
    m_player->setMedia(Controls::m_defaultSound);
    m_player->setVolume(100);

    /* Signal-Slot connections:
     * For every significant event on the controls object, e.g., adding to the
     * score or changing a division, we emit a signal saying we have done so,
     * and then connect that signal here to a slot on the scoreboard object.
    */

    connect(timer, &QTimer::timeout, this, &Controls::updateClock);

    connect(this, &Controls::competitor1PointsChanged,
            board, &Scoreboard::setCompetitor1Points);
    connect(this, &Controls::competitor1AdvantagesChanged,
            board, &Scoreboard::setCompetitor1Advantages);
    connect(this, &Controls::competitor1PenaltiesChanged,
            board, &Scoreboard::setCompetitor1Penalties);

    connect(this, &Controls::competitor2PointsChanged,
            board, &Scoreboard::setCompetitor2Points);
    connect(this, &Controls::competitor2AdvantagesChanged,
            board, &Scoreboard::setCompetitor2Advantages);
    connect(this, &Controls::competitor2PenaltiesChanged,
            board, &Scoreboard::setCompetitor2Penalties);

    connect(this, &Controls::competitor1NameChanged,
            board, &Scoreboard::setCompetitor1Name);
    connect(this, &Controls::competitor2NameChanged,
            board, &Scoreboard::setCompetitor2Name);

    connect(this, &Controls::timerUpdated,
            board, &Scoreboard::setTimeLabel);
    connect(this, &Controls::divisionUpdated,
            board, &Scoreboard::setDivisionLabel);
    connect(this, &Controls::beltUpdated,
            board, &Scoreboard::setBeltLabel);

    connect(this, &Controls::logoUpdated,
            board, &Scoreboard::setLogo);

    connect(this, &Controls::matchReset,
            board, &Scoreboard::resetScores);

    timer->start(1000); // every 1 second
}

Controls::~Controls()
{
    delete ui;
    delete board;
    delete m_player;
    delete timer;
}

void Controls::modify_points(QLabel *label, int amount)
{
    int oldValue = label->text().toInt();
    int newValue = oldValue + amount < 0 ? 0 : oldValue + amount;
    label->setText(QString::number(newValue));
}

void Controls::stopClock()
{

}

QString Controls::calcNewTimeString()
{
    clockMinutes = totalTime / 60;
    clockSeconds = totalTime % 60;

    QString minutes = QString::number(clockMinutes);
    QString seconds = QString::number(clockSeconds);

    if (clockSeconds < 10) {
        QString extraZero = "0";
        seconds.prepend(extraZero);
    }

    QString newTime = minutes + ":" + seconds;
    return newTime;
}

void Controls::updateDisplay()
{
    QString newTime = calcNewTimeString();
    // Update label
    ui->timerLabel->setText(newTime);

    // emit signals saying what we did
    emit timerUpdated(newTime);
    emit divisionUpdated(m_currentDivision);
    emit beltUpdated(m_currentBelt);
}

void Controls::updateClock()
{
    if (matchDone) {
        qDebug() << "updateClock(): matchDone = true condition hit.";
        /* GOTCHA: just update the display and stop here, this handles
         * case where match was started and clock was ticking and
         * user hits reset button without hitting pause first.
         */
        updateDisplay();
        return;
    }

    if (totalTime <= 0) {
        qDebug() << "updateClock(): totalTime <=0 condition hit.";
        Controls::playSound();
        matchDone = true;
        return; // stop here
    }

    if (matchStarted && clockRunning) {
        qDebug() << "updateClock(): matchStarted && clockRunning = true condition hit.";
        totalTime--;
        updateDisplay();
    }

}

void Controls::playSound()
{
    if (ui->OnRadioButton->isChecked()) {
        m_player->play();
    }
    ui->playPauseButton->setDisabled(true);

}

// TODO: combine disable/enable to single flip state call
void Controls::disableControls()
{
    ui->divisionComboBox->setDisabled(true);
}

void Controls::enableControls()
{
    ui->playPauseButton->setDisabled(false);
    ui->divisionComboBox->setDisabled(false);
}

void Controls::closeEvent(QCloseEvent *event)
{
    qDebug() << "Close event caught on control window.";
    QMessageBox::StandardButton resBtn = QMessageBox::question(this,
                                      "Openroll",
                                      "Are you sure?",
                                      QMessageBox::No | QMessageBox::Yes,
                                      QMessageBox::No);
    if (resBtn != QMessageBox::Yes) {
        event->ignore();
    }
    else {
        event->accept();
    }
}

void Controls::on_c1Add2Button_pressed()
{
    QLabel *label = ui->c1PointsLabel;
    modify_points(label, 2);
    emit competitor1PointsChanged(2);
}

void Controls::on_c1Del2Button_pressed()
{
    QLabel *label = ui->c1PointsLabel;
    modify_points(label, -2);
    emit competitor1PointsChanged(-2);

}

void Controls::on_c1Add3Button_pressed()
{
    QLabel *label = ui->c1PointsLabel;
    modify_points(label, 3);
    emit competitor1PointsChanged(3);
}

void Controls::on_divisionComboBox_currentIndexChanged(int index)
{
    switch (index) {
    case 0: // MIGHTY MITE 1
        m_currentDivision = "MIGHTY MITE 1 - AGE 4";
        m_currentBelt = "ALL BELTS";
        totalTime = 2 * 60;
        break;
    case 1: // MIGHTY MITE 2
        m_currentDivision = "MIGHTY MITE 2 - AGE 5";
        m_currentBelt = "ALL BELTS";
        totalTime = 2 * 60;
        break;
    case 2: // MIGHTY MITE 3
        m_currentDivision = "MIGHTY MITE 3 - AGE 6";
        m_currentBelt = "ALL BELTS";
        totalTime = 2 * 60;
        break;
    case 3: // PEEWEE 1
        m_currentDivision = "PEEWEE 1 - AGE 7";
        m_currentBelt = "ALL BELTS";
        totalTime = 3 * 60;
        break;
    case 4: // PEEWEE 2
        m_currentDivision = "PEEWEE 2 - AGE 8";
        m_currentBelt = "ALL BELTS";
        totalTime = 3 * 60;
        break;
    case 5: // PEEWEE 3
        m_currentDivision = "PEEWEE 3 - AGE 9";
        m_currentBelt = "ALL BELTS";
        totalTime = 3 * 60; // 3 minutes
        break;
    case 6: // JUNIOR 1
        m_currentDivision = "JUNIOR 1 - AGE 10";
        m_currentBelt = "ALL BELTS";
        totalTime = 4 * 60;
        break;
    case 7: // JUNIOR 2
        m_currentDivision = "JUNIOR 2 - AGE 11";
        m_currentBelt = "ALL BELTS";
        totalTime = 4 * 60;
        break;
    case 8: // JUNIOR 3
        m_currentDivision = "JUNIOR 3 - AGE 12";
        m_currentBelt = "ALL BELTS";
        totalTime = 4 * 60;
        break;
    case 9: // TEEN 1
        m_currentDivision = "TEEN 1 - AGE 13";
        m_currentBelt = "ALL BELTS";
        totalTime = 4 * 60;
        break;
    case 10: // TEEN 2
        m_currentDivision = "TEEN 2 - AGE 14";
        m_currentBelt = "ALL BELTS";
        totalTime = 4 * 60;
        break;
    case 11: // TEEN 3
        m_currentDivision = "TEEN 3 - AGE 15";
        m_currentBelt = "ALL BELTS";
        totalTime = 4 * 60;
        break;
    case 12: // JUVENILE 1
        m_currentDivision = "JUVENILE 1 - AGE 16";
        m_currentBelt = "ALL BELTS";
        totalTime = 5 * 60;
        break;
    case 13: // JUVENILE 2
        m_currentDivision = "JUVENILE 2 - AGE 17";
        m_currentBelt = "ALL BELTS";
        totalTime = 5 * 60;
        break;
    case 14: // ADULT 1A
        m_currentDivision = "ADULT 1A - AGE 18-29";
        m_currentBelt = "WHITE";
        totalTime = 5 * 60;
        break;
    case 15: // ADULT 1B
        m_currentDivision = "ADULT 1B - AGE 18-29";
        m_currentBelt = "BLUE";
        totalTime = 6 * 60;
        break;
    case 16: // ADULT 1C
        m_currentDivision = "ADULT 1C - AGE 18-29";
        m_currentBelt = "PURPLE";
        totalTime = 7 * 60;
        break;
    case 17: // ADULT 1D
        m_currentDivision = "ADULT 1D - AGE 18-29";
        m_currentBelt = "BROWN";
        totalTime = 8 * 60;
        break;
    case 18: // ADULT 1E
        m_currentDivision = "ADULT 1E - AGE 18-29";
        m_currentBelt = "BLACK";
        totalTime = 10 * 60;
        break;
    case 19: // MASTER 1A
        m_currentDivision = "MASTER 1A - AGE 30-35";
        m_currentBelt = "WHITE";
        totalTime = 5 * 60;
        break;
    case 20: // MASTER 1B
        m_currentDivision = "MASTER 1B - AGE 30-35";
        m_currentBelt = "BLUE";
        totalTime = 5 * 60;
        break;
    case 21: // MASTER 1C
        m_currentDivision = "MASTER 1C - AGE 30-35";
        m_currentBelt = "PURPLE";
        totalTime = 6 * 60;
        break;
    case 22: // MASTER 1D
        m_currentDivision = "MASTER 1D - AGE 30-35";
        m_currentBelt = "BROWN";
        totalTime = 6 * 60;
        break;
    case 23: // MASTER 1E
        m_currentDivision = "MASTER 1E - AGE 30-35";
        m_currentBelt = "BLACK";
        totalTime = 6 * 60;
        break;
    case 24: // MASTER 2
        m_currentDivision = "MASTER 2 - AGE 36-40";
        m_currentBelt = "ALL BELTS";
        totalTime = 5 * 60;
        break;
    case 25: // MASTER 3
        m_currentDivision = "MASTER 3 - AGE 41-45";
        m_currentBelt = "ALL BELTS";
        totalTime = 5 * 60;
        break;
    case 26: // MASTER 4
        m_currentDivision = "MASTER 4 - AGE 46-50";
        m_currentBelt = "ALL BELTS";
        totalTime = 5 * 60;
        break;
    case 27: // MASTER 5
        m_currentDivision = "MASTER 5 - AGE 51-55+";
        m_currentBelt = "ALL BELTS";
        totalTime = 5 * 60;
        break;
    case 28: // MASTER 6
        m_currentDivision = "MASTER 6 - AGE 56+";
        m_currentBelt = "ALL BELTS";
        totalTime = 5 * 60;
        break;
    default: // NO MATCH FROM DROPDOWN CASE!
        m_currentDivision = "UNKNOWN";
        m_currentBelt = "UNKNOWN";
        totalTime = 5 * 60;
        break;
    };
}

void Controls::on_soundComboBox_currentIndexChanged(const QString &sound)
{
   QString f = "qrc:///sounds/" + sound;
   qDebug() << "Changing to sound file: " << f;
   m_player->setMedia(QUrl(f));
   //playSound();
}

void Controls::on_loadLogoButton_pressed()
{
    QString logoFilename = QFileDialog::getOpenFileName(this,
                                        tr("Open Image"), ".",
                        tr("Image Files (*.png *.jpg *.bmp)"));
    if (!logoFilename.isEmpty()) {
        emit logoUpdated(logoFilename);
    }
}

void Controls::on_c1FlagComboBox_currentIndexChanged(int index)
{
    // TODO: c1 -- pick new flag from dropdown
    emit competitor1FlagChanged(index);

}

void Controls::on_c1CustomFlagButton_pressed()
{
    // TODO: c1 -- pick custom flag to load
    QString flag = "";
    emit competitor1CustomFlagLoaded(flag);
}

void Controls::on_c2CustomLogoButton_pressed()
{
    // TODO: c2 -- load custom logo
    QString flag = "";
    emit competitor2CustomFlagLoaded(flag);
}

void Controls::on_c2FlagComboBox_currentIndexChanged(int index)
{
    // TODO: c2 -- pick new flag from dropdown
    emit competitor2FlagChanged(index);

}

void Controls::on_playPauseButton_pressed()
{
    qDebug() << "Control:on_play_button_clicked() entry state: " <<
                "matchStarted: " << matchStarted << ", " <<
                "clockRunning: " << clockRunning << ", " <<
                "clockPaused: " << clockPaused;

    if (!matchStarted) {
        /* current image is play icon, flip to pause icon and
         * disable any controls we don't want user pressing
         * after match has officially started.
         */
        matchStarted = true;
        clockRunning = true;
        clockPaused = false;
        ui->playPauseButton->setIcon(QIcon(":/ui/pause"));
        disableControls();
    }
    else {
        /* matchStarted is true, meaning user pressed play button
         * already to start the match. After first time
         * on_play_button_clicked() is fired, the code path will keep
         * hitting this 'else' path until the state of the match is
         * flipped to matchStarted = false
         */
        if (clockPaused) { // current image is 'play' icon, flip to pause icon
            ui->playPauseButton->setIcon(QIcon(":/ui/pause"));
            clockPaused = false;
            clockRunning = true;
        }
        else if (clockRunning) { // current image is 'pause' icon
            // stop clock and flip image to 'play'
            ui->playPauseButton->setIcon(QIcon(":/ui/play"));
            clockRunning = false;
            clockPaused = true;
        }
    }

    qDebug() << "Control:on_play_button_clicked() exit state: " <<
                "matchStarted: " << matchStarted << ", " <<
                "clockRunning: " << clockRunning << ", " <<
                "clockPaused: " << clockPaused;
}

void Controls::on_resetButton_pressed()
{
    ui->c1NameLabel->setText("Competitor 1");
    ui->c1PointsLabel->setText("0");
    ui->c1AdvantagesLabel->setText("0");
    ui->c1PenaltiesLabel->setText("0");
    ui->c1PointsLabel->setText("0");

    ui->c2NameLabel->setText("Competitor 2");
    ui->c2PointsLabel->setText("0");
    ui->c2AdvantagesLabel->setText("0");
    ui->c2PenaltiesLabel->setText("0");
    ui->c2PointsLabel->setText("0");

    ui->playPauseButton->setIcon(QIcon(":/ui/play"));

    // re-enable any controls that were disabled during running of match timer
    enableControls();

    // TODO: reset timings as well here
    /* Give current index to our map to get total seconds for reset value,
     * because we to keep the same division and timings when reset is pressed rather
     * than requiring user to change those each time they hit reset.
     */
    totalTime = divisionIdxToTimeMap[ui->divisionComboBox->currentIndex()];
    clockMinutes = totalTime / 60;
    clockSeconds = totalTime % 60;

    // now emit that match has been fully reset
    emit matchReset();
}

void Controls::on_c1Del3Button_pressed()
{
    QLabel *label = ui->c1PointsLabel;
    modify_points(label, -3);
    emit competitor1PointsChanged(-3);
}

void Controls::on_c1Add4Button_pressed()
{
    QLabel *label = ui->c1PointsLabel;
    modify_points(label, 4);
    emit competitor1PointsChanged(4);

}

void Controls::on_c1Del4Button_pressed()
{
    QLabel *label = ui->c1PointsLabel;
    modify_points(label, -4);
    emit competitor1PointsChanged(-4);
}

void Controls::on_c1AddAButton_pressed()
{
    QLabel *label = ui->c1AdvantagesLabel;
    modify_points(label, 1);
    emit competitor1AdvantagesChanged(1);
}

void Controls::on_c1DelAButton_pressed()
{
    QLabel *label = ui->c1AdvantagesLabel;
    modify_points(label, -1);
    emit competitor1AdvantagesChanged(-1);

}

void Controls::on_c1AddPButton_pressed()
{
    QLabel *label = ui->c1PenaltiesLabel;
    modify_points(label, 1);
    emit competitor1PenaltiesChanged(1);
}

void Controls::on_c1DelPButton_pressed()
{
    QLabel *label = ui->c1PenaltiesLabel;
    modify_points(label, -1);
    emit competitor1PenaltiesChanged(-1);
}

void Controls::on_c2Add2Button_pressed()
{
   QLabel *label = ui->c2PointsLabel;
   modify_points(label, 2);
   emit competitor2PointsChanged(2);
}

void Controls::on_c2Del2Button_pressed()
{
   QLabel *label = ui->c2PointsLabel;
   modify_points(label, -2);
   emit competitor2PointsChanged(-2);
}

void Controls::on_c2Add3Button_pressed()
{
   QLabel *label = ui->c2PointsLabel;
   modify_points(label, 3);
   emit competitor2PointsChanged(3);
}

void Controls::on_c2Del3Button_pressed()
{
   QLabel *label = ui->c2PointsLabel;
   modify_points(label, -3);
   emit competitor2PointsChanged(-3);
}

void Controls::on_c2Add4Button_pressed()
{

   QLabel *label = ui->c2PointsLabel;
   modify_points(label, 4);
   emit competitor2PointsChanged(4);
}

void Controls::on_c2Del4Button_pressed()
{
   QLabel *label = ui->c2PointsLabel;
   modify_points(label, -4);
   emit competitor2PointsChanged(-4);
}

void Controls::on_c2AddAButton_pressed()
{
   QLabel *label = ui->c2AdvantagesLabel;
   modify_points(label, 1);
   emit competitor2AdvantagesChanged(1);
}

void Controls::on_c2DelAButton_pressed()
{
   QLabel *label = ui->c2AdvantagesLabel;
   modify_points(label, -1);
   emit competitor2AdvantagesChanged(-1);
}

void Controls::on_c2AddPButton_pressed()
{
   QLabel *label = ui->c2PenaltiesLabel;
   modify_points(label, 1);
   emit competitor2PenaltiesChanged(1);
}

void Controls::on_c2DelPButton_pressed()
{
   QLabel *label = ui->c2PenaltiesLabel;
   modify_points(label, -1);
   emit competitor2PenaltiesChanged(-1);
}

void Controls::on_c1LineEdit_textEdited(const QString &str)
{
    ui->c1NameLabel->setText(str);
    emit competitor1NameChanged(str);
}

void Controls::on_c2LineEdit_textEdited(const QString &str)
{
   ui->c2NameLabel->setText(str);
   emit competitor2NameChanged(str);
}

void Controls::on_testSoundButton_pressed()
{
    playSound();
}
