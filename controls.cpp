#include "controls.h"
#include "openroll-config.h"
#include "scoreboard.h"
#include "ui_controls.h"
#ifdef LOGGER
    #include "logger.h"
#endif
#include <map>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QStringList>
#include <QTime>
#include <QtWidgets>

/**
 * @brief Controls::Controls
 * @param parent QWidget
 */
Controls::Controls(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Controls)
{
    ui->setupUi(this);
    setWindowTitle("Openroll - Controls - " + OPENROLL_VERSION);
    // Pass flag to make board an independent window is required or it will draw inside controls window
    Scoreboard *board = new Scoreboard(this, Qt::WindowFlags(Qt::Window));
    board->setWindowTitle("Openroll - Scoreboard - " + OPENROLL_VERSION);
    board->setAttribute(Qt::WA_DeleteOnClose);
    board->show();

    divisionIdxToTimeMap[0] = TWO_MINUTES;
    divisionIdxToTimeMap[1] = TWO_MINUTES;
    divisionIdxToTimeMap[2] = TWO_MINUTES;
    divisionIdxToTimeMap[3] = THREE_MINUTES;
    divisionIdxToTimeMap[4] = THREE_MINUTES;
    divisionIdxToTimeMap[5] = THREE_MINUTES;
    divisionIdxToTimeMap[6] = FOUR_MINUTES;
    divisionIdxToTimeMap[7] = FOUR_MINUTES;
    divisionIdxToTimeMap[8] = FOUR_MINUTES;
    divisionIdxToTimeMap[9] = FOUR_MINUTES;
    divisionIdxToTimeMap[10] = FOUR_MINUTES;
    divisionIdxToTimeMap[11] = FOUR_MINUTES;
    divisionIdxToTimeMap[12] = FIVE_MINUTES;
    divisionIdxToTimeMap[13] = FIVE_MINUTES;
    divisionIdxToTimeMap[14] = FIVE_MINUTES;
    divisionIdxToTimeMap[15] = SIX_MINUTES;
    divisionIdxToTimeMap[16] = SEVEN_MINUTES;
    divisionIdxToTimeMap[17] = EIGHT_MINUTES;
    divisionIdxToTimeMap[18] = TEN_MINUTES;
    divisionIdxToTimeMap[19] = FIVE_MINUTES;
    divisionIdxToTimeMap[20] = FIVE_MINUTES;
    divisionIdxToTimeMap[21] = SIX_MINUTES;
    divisionIdxToTimeMap[22] = SIX_MINUTES;
    divisionIdxToTimeMap[23] = SIX_MINUTES;
    divisionIdxToTimeMap[24] = FIVE_MINUTES;
    divisionIdxToTimeMap[25] = FIVE_MINUTES;
    divisionIdxToTimeMap[26] = FIVE_MINUTES;
    divisionIdxToTimeMap[27] = FIVE_MINUTES;
    divisionIdxToTimeMap[28] = FIVE_MINUTES;

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

    connect(this, &Controls::competitor1FlagChanged, board, &Scoreboard::setC1Flag);

    connect(this, &Controls::competitor2FlagChanged, board, &Scoreboard::setC2Flag);

    populateFlagDropDowns();

    timer->start(1000); // every 1 second
}

/**
 * @brief Controls::~Controls
 */
Controls::~Controls()
{
    qDebug() << "Controls destructor called.";
    delete ui;
}

/**
 * @brief Controls::modify_points
 * @param label QLabel
 * @param amount int
 */
void Controls::modify_points(QLabel *label, int amount)
{
    int oldValue = label->text().toInt();
    int newValue = oldValue + amount < 0 ? 0 : oldValue + amount;
    label->setText(QString::number(newValue));
}

/**
 * @brief Controls::calcNewTimeString
 * @return QString
 */
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

/**
 * @brief Controls::updateDisplay
 */
void Controls::updateDisplay()
{
    QString newTime = calcNewTimeString();

    // Update label on Controls windows
    ui->timerLabel->setText(newTime);

    // emit signals saying what we did and/or what should update
    emit timerUpdated(newTime);
    emit divisionUpdated(m_currentDivision);
    emit beltUpdated(m_currentBelt);
}

/**
 * @brief Controls::updateClock
 */
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

/**
 * @brief Controls::playSound
 */
void Controls::playSound()
{
    if (ui->OnRadioButton->isChecked()) {
        m_player->play();
    }
    //ui->playPauseButton->setDisabled(true);

}

/**
 * @brief Controls::disableControls
 */
void Controls::disableControls()
{
    ui->divisionComboBox->setDisabled(true);
}

/**
 * @brief Controls::enableControls
 */
void Controls::enableControls()
{
    ui->playPauseButton->setDisabled(false);
    ui->divisionComboBox->setDisabled(false);
}

/**
 * @brief Controls::closeEvent
 * @param event QCloseEvent
 */
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

/**
 * @brief Controls::on_c1Add2Button_pressed
 */
void Controls::on_c1Add2Button_pressed()
{
    QLabel *label = ui->c1PointsLabel;
    modify_points(label, 2);
    emit competitor1PointsChanged(2);
}

/**
 * @brief Controls::on_c1Del2Button_pressed
 */
void Controls::on_c1Del2Button_pressed()
{
    QLabel *label = ui->c1PointsLabel;
    modify_points(label, -2);
    emit competitor1PointsChanged(-2);

}

/**
 * @brief Controls::on_c1Add3Button_pressed
 */
void Controls::on_c1Add3Button_pressed()
{
    QLabel *label = ui->c1PointsLabel;
    modify_points(label, 3);
    emit competitor1PointsChanged(3);
}

/**
 * @brief Controls::on_divisionComboBox_currentIndexChanged
 * @param index int
 */
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
    }

    // Call here or display won't be updated if match is not running
    updateDisplay();
}

/**
 * @brief Controls::on_soundComboBox_currentIndexChanged
 * @param sound QString
 */
void Controls::on_soundComboBox_currentIndexChanged(const QString &sound)
{
   QString f = "qrc:///sounds/" + sound;
   qDebug() << "Changing to sound file: " << f;
   m_player->setMedia(QUrl(f));
}

/**
 * @brief Controls::populateFlagDropDowns
 *
 * Add all the countries and related flag icons to
 * each competitor dropdown.
 */
void Controls::populateFlagDropDowns()
{
    // TODO: use a thread/parallel to do this for even moar speed?
    countryToResourceMap.insert("ad", ":/small-flags/ad-small");
    countryToResourceMap.insert("ae", ":/small-flags/ae-small");
    countryToResourceMap.insert("af", ":/small-flags/af-small");
    countryToResourceMap.insert("ag", ":/small-flags/ag-small");
    countryToResourceMap.insert("al", ":/small-flags/al-small");
    countryToResourceMap.insert("am", ":/small-flags/am-small");
    countryToResourceMap.insert("ao", ":/small-flags/ao-small");
    countryToResourceMap.insert("ar", ":/small-flags/ar-small");
    countryToResourceMap.insert("at", ":/small-flags/at-small");
    countryToResourceMap.insert("au", ":/small-flags/au-small");
    countryToResourceMap.insert("az", ":/small-flags/az-small");
    countryToResourceMap.insert("ba", ":/small-flags/ba-small");
    countryToResourceMap.insert("bb", ":/small-flags/bb-small");
    countryToResourceMap.insert("bd", ":/small-flags/bd-small");
    countryToResourceMap.insert("be", ":/small-flags/be-small");
    countryToResourceMap.insert("bf", ":/small-flags/bf-small");
    countryToResourceMap.insert("bg", ":/small-flags/bg-small");
    countryToResourceMap.insert("bh", ":/small-flags/bh-small");
    countryToResourceMap.insert("bi", ":/small-flags/bi-small");
    countryToResourceMap.insert("bj", ":/small-flags/bj-small");
    countryToResourceMap.insert("bn", ":/small-flags/bn-small");
    countryToResourceMap.insert("bo", ":/small-flags/bo-small");
    countryToResourceMap.insert("br", ":/small-flags/br-small");
    countryToResourceMap.insert("bs", ":/small-flags/bs-small");
    countryToResourceMap.insert("bt", ":/small-flags/bt-small");
    countryToResourceMap.insert("bw", ":/small-flags/bw-small");
    countryToResourceMap.insert("by", ":/small-flags/by-small");
    countryToResourceMap.insert("bz", ":/small-flags/bz-small");
    countryToResourceMap.insert("ca", ":/small-flags/ca-small");
    countryToResourceMap.insert("cd", ":/small-flags/cd-small");
    countryToResourceMap.insert("cf", ":/small-flags/cf-small");
    countryToResourceMap.insert("cg", ":/small-flags/cg-small");
    countryToResourceMap.insert("ch", ":/small-flags/ch-small");
    countryToResourceMap.insert("ci", ":/small-flags/ci-small");
    countryToResourceMap.insert("cl", ":/small-flags/cl-small");
    countryToResourceMap.insert("cm", ":/small-flags/cm-small");
    countryToResourceMap.insert("cn", ":/small-flags/cn-small");
    countryToResourceMap.insert("co", ":/small-flags/co-small");
    countryToResourceMap.insert("cr", ":/small-flags/cr-small");
    countryToResourceMap.insert("cu", ":/small-flags/cu-small");
    countryToResourceMap.insert("cv", ":/small-flags/cv-small");
    countryToResourceMap.insert("cy", ":/small-flags/cy-small");
    countryToResourceMap.insert("cz", ":/small-flags/cz-small");
    countryToResourceMap.insert("de", ":/small-flags/de-small");
    countryToResourceMap.insert("dj", ":/small-flags/dj-small");
    countryToResourceMap.insert("dk", ":/small-flags/dk-small");
    countryToResourceMap.insert("dm", ":/small-flags/dm-small");
    countryToResourceMap.insert("do", ":/small-flags/do-small");
    countryToResourceMap.insert("dz", ":/small-flags/dz-small");
    countryToResourceMap.insert("ec", ":/small-flags/ec-small");
    countryToResourceMap.insert("ee", ":/small-flags/ee-small");
    countryToResourceMap.insert("eg", ":/small-flags/eg-small");
    countryToResourceMap.insert("eh", ":/small-flags/eh-small");
    countryToResourceMap.insert("er", ":/small-flags/er-small");
    countryToResourceMap.insert("es", ":/small-flags/es-small");
    countryToResourceMap.insert("et", ":/small-flags/et-small");
    countryToResourceMap.insert("fi", ":/small-flags/fi-small");
    countryToResourceMap.insert("fj", ":/small-flags/fj-small");
    countryToResourceMap.insert("fm", ":/small-flags/fm-small");
    countryToResourceMap.insert("fr", ":/small-flags/fr-small");
    countryToResourceMap.insert("ga", ":/small-flags/ga-small");
    countryToResourceMap.insert("gb", ":/small-flags/gb-small");
    countryToResourceMap.insert("gd", ":/small-flags/gd-small");
    countryToResourceMap.insert("ge", ":/small-flags/ge-small");
    countryToResourceMap.insert("gh", ":/small-flags/gh-small");
    countryToResourceMap.insert("gm", ":/small-flags/gm-small");
    countryToResourceMap.insert("gn", ":/small-flags/gn-small");
    countryToResourceMap.insert("gq", ":/small-flags/gq-small");
    countryToResourceMap.insert("gr", ":/small-flags/gr-small");
    countryToResourceMap.insert("gt", ":/small-flags/gt-small");
    countryToResourceMap.insert("gw", ":/small-flags/gw-small");
    countryToResourceMap.insert("gy", ":/small-flags/gy-small");
    countryToResourceMap.insert("hn", ":/small-flags/hn-small");
    countryToResourceMap.insert("hr", ":/small-flags/hr-small");
    countryToResourceMap.insert("ht", ":/small-flags/ht-small");
    countryToResourceMap.insert("hu", ":/small-flags/hu-small");
    countryToResourceMap.insert("id", ":/small-flags/id-small");
    countryToResourceMap.insert("ie", ":/small-flags/ie-small");
    countryToResourceMap.insert("il", ":/small-flags/il-small");
    countryToResourceMap.insert("in", ":/small-flags/in-small");
    countryToResourceMap.insert("iq", ":/small-flags/iq-small");
    countryToResourceMap.insert("ir", ":/small-flags/ir-small");
    countryToResourceMap.insert("is", ":/small-flags/is-small");
    countryToResourceMap.insert("it", ":/small-flags/it-small");
    countryToResourceMap.insert("jm", ":/small-flags/jm-small");
    countryToResourceMap.insert("jo", ":/small-flags/jo-small");
    countryToResourceMap.insert("jp", ":/small-flags/jp-small");
    countryToResourceMap.insert("ke", ":/small-flags/ke-small");
    countryToResourceMap.insert("kg", ":/small-flags/kg-small");
    countryToResourceMap.insert("kh", ":/small-flags/kh-small");
    countryToResourceMap.insert("ki", ":/small-flags/ki-small");
    countryToResourceMap.insert("km", ":/small-flags/km-small");
    countryToResourceMap.insert("kn", ":/small-flags/kn-small");
    countryToResourceMap.insert("kp", ":/small-flags/kp-small");
    countryToResourceMap.insert("kr", ":/small-flags/kr-small");
    countryToResourceMap.insert("ks", ":/small-flags/ks-small");
    countryToResourceMap.insert("kw", ":/small-flags/kw-small");
    countryToResourceMap.insert("kz", ":/small-flags/kz-small");
    countryToResourceMap.insert("la", ":/small-flags/la-small");
    countryToResourceMap.insert("lb", ":/small-flags/lb-small");
    countryToResourceMap.insert("lc", ":/small-flags/lc-small");
    countryToResourceMap.insert("li", ":/small-flags/li-small");
    countryToResourceMap.insert("lk", ":/small-flags/lk-small");
    countryToResourceMap.insert("lr", ":/small-flags/lr-small");
    countryToResourceMap.insert("ls", ":/small-flags/ls-small");
    countryToResourceMap.insert("lt", ":/small-flags/lt-small");
    countryToResourceMap.insert("lu", ":/small-flags/lu-small");
    countryToResourceMap.insert("lv", ":/small-flags/lv-small");
    countryToResourceMap.insert("ly", ":/small-flags/ly-small");
    countryToResourceMap.insert("ma", ":/small-flags/ma-small");
    countryToResourceMap.insert("mc", ":/small-flags/mc-small");
    countryToResourceMap.insert("md", ":/small-flags/md-small");
    countryToResourceMap.insert("me", ":/small-flags/me-small");
    countryToResourceMap.insert("mg", ":/small-flags/mg-small");
    countryToResourceMap.insert("mh", ":/small-flags/mh-small");
    countryToResourceMap.insert("mk", ":/small-flags/mk-small");
    countryToResourceMap.insert("ml", ":/small-flags/ml-small");
    countryToResourceMap.insert("mm", ":/small-flags/mm-small");
    countryToResourceMap.insert("mn", ":/small-flags/mn-small");
    countryToResourceMap.insert("mr", ":/small-flags/mr-small");
    countryToResourceMap.insert("mt", ":/small-flags/mt-small");
    countryToResourceMap.insert("mu", ":/small-flags/mu-small");
    countryToResourceMap.insert("mv", ":/small-flags/mv-small");
    countryToResourceMap.insert("mw", ":/small-flags/mw-small");
    countryToResourceMap.insert("mx", ":/small-flags/mx-small");
    countryToResourceMap.insert("my", ":/small-flags/my-small");
    countryToResourceMap.insert("mz", ":/small-flags/mz-small");
    countryToResourceMap.insert("na", ":/small-flags/na-small");
    countryToResourceMap.insert("ne", ":/small-flags/ne-small");
    countryToResourceMap.insert("ng", ":/small-flags/ng-small");
    countryToResourceMap.insert("ni", ":/small-flags/ni-small");
    countryToResourceMap.insert("nl", ":/small-flags/nl-small");
    countryToResourceMap.insert("no", ":/small-flags/no-small");
    countryToResourceMap.insert("np", ":/small-flags/np-small");
    countryToResourceMap.insert("nr", ":/small-flags/nr-small");
    countryToResourceMap.insert("nz", ":/small-flags/nz-small");
    countryToResourceMap.insert("om", ":/small-flags/om-small");
    countryToResourceMap.insert("pa", ":/small-flags/pa-small");
    countryToResourceMap.insert("pe", ":/small-flags/pe-small");
    countryToResourceMap.insert("pg", ":/small-flags/pg-small");
    countryToResourceMap.insert("ph", ":/small-flags/ph-small");
    countryToResourceMap.insert("pk", ":/small-flags/pk-small");
    countryToResourceMap.insert("pl", ":/small-flags/pl-small");
    countryToResourceMap.insert("pt", ":/small-flags/pt-small");
    countryToResourceMap.insert("pw", ":/small-flags/pw-small");
    countryToResourceMap.insert("py", ":/small-flags/py-small");
    countryToResourceMap.insert("qa", ":/small-flags/qa-small");
    countryToResourceMap.insert("ro", ":/small-flags/ro-small");
    countryToResourceMap.insert("rs", ":/small-flags/rs-small");
    countryToResourceMap.insert("ru", ":/small-flags/ru-small");
    countryToResourceMap.insert("rw", ":/small-flags/rw-small");
    countryToResourceMap.insert("sa", ":/small-flags/sa-small");
    countryToResourceMap.insert("sb", ":/small-flags/sb-small");
    countryToResourceMap.insert("sc", ":/small-flags/sc-small");
    countryToResourceMap.insert("sd", ":/small-flags/sd-small");
    countryToResourceMap.insert("se", ":/small-flags/se-small");
    countryToResourceMap.insert("sg", ":/small-flags/sg-small");
    countryToResourceMap.insert("si", ":/small-flags/si-small");
    countryToResourceMap.insert("sk", ":/small-flags/sk-small");
    countryToResourceMap.insert("sl", ":/small-flags/sl-small");
    countryToResourceMap.insert("sm", ":/small-flags/sm-small");
    countryToResourceMap.insert("sn", ":/small-flags/sn-small");
    countryToResourceMap.insert("so", ":/small-flags/so-small");
    countryToResourceMap.insert("sr", ":/small-flags/sr-small");
    countryToResourceMap.insert("st", ":/small-flags/st-small");
    countryToResourceMap.insert("sv", ":/small-flags/sv-small");
    countryToResourceMap.insert("sy", ":/small-flags/sy-small");
    countryToResourceMap.insert("sz", ":/small-flags/sz-small");
    countryToResourceMap.insert("td", ":/small-flags/td-small");
    countryToResourceMap.insert("tg", ":/small-flags/tg-small");
    countryToResourceMap.insert("th", ":/small-flags/th-small");
    countryToResourceMap.insert("tj", ":/small-flags/tj-small");
    countryToResourceMap.insert("tl", ":/small-flags/tl-small");
    countryToResourceMap.insert("tm", ":/small-flags/tm-small");
    countryToResourceMap.insert("tn", ":/small-flags/tn-small");
    countryToResourceMap.insert("to", ":/small-flags/to-small");
    countryToResourceMap.insert("tr", ":/small-flags/tr-small");
    countryToResourceMap.insert("tt", ":/small-flags/tt-small");
    countryToResourceMap.insert("tv", ":/small-flags/tv-small");
    countryToResourceMap.insert("tw", ":/small-flags/tw-small");
    countryToResourceMap.insert("tz", ":/small-flags/tz-small");
    countryToResourceMap.insert("ua", ":/small-flags/ua-small");
    countryToResourceMap.insert("ug", ":/small-flags/ug-small");
    countryToResourceMap.insert("us", ":/small-flags/us-small");
    countryToResourceMap.insert("uy", ":/small-flags/uy-small");
    countryToResourceMap.insert("uz", ":/small-flags/uz-small");
    countryToResourceMap.insert("va", ":/small-flags/va-small");
    countryToResourceMap.insert("vc", ":/small-flags/vc-small");
    countryToResourceMap.insert("ve", ":/small-flags/ve-small");
    countryToResourceMap.insert("vn", ":/small-flags/vn-small");
    countryToResourceMap.insert("vu", ":/small-flags/vu-small");
    countryToResourceMap.insert("ws", ":/small-flags/ws-small");
    countryToResourceMap.insert("ye", ":/small-flags/ye-small");
    countryToResourceMap.insert("za", ":/small-flags/za-small");
    countryToResourceMap.insert("zm", ":/small-flags/zm-small");
    countryToResourceMap.insert("zw", ":/small-flags/zw-small");

    QMap<QString, QString>::const_iterator i = countryToResourceMap.constBegin();
    int j = 0;
    while (i != countryToResourceMap.constEnd()) {
        ui->c1FlagComboBox->insertItem(j, QIcon(i.value()), i.key());
        ui->c2FlagComboBox->insertItem(j, QIcon(i.value()), i.key());
        ++j;
        ++i;
    }
}

/**
 * @brief Controls::on_loadLogoButton_pressed
 */
void Controls::on_loadLogoButton_pressed()
{
    QString logoFilename = QFileDialog::getOpenFileName(this,
                                        tr("Open Image"), ".",
                        tr("Image Files (*.png *.jpg *.bmp)"));
    if (!logoFilename.isEmpty()) {
        emit logoUpdated(logoFilename);
    }
}

/**
 * @brief Controls::on_c1CustomFlagButton_pressed
 */
void Controls::on_c1CustomFlagButton_pressed()
{
    QString customLogo = QFileDialog::getOpenFileName(this,
                                        tr("Open Image"), ".",
                        tr("Image Files (*.png *.jpg *.bmp)"));

    qDebug() << "on_c1CustomFlagButton_pressed(): using customLogo string: " << customLogo;

    if (!customLogo.isEmpty()) {
        emit competitor1FlagChanged(customLogo);
    }
}

/**
 * @brief Controls::on_c2CustomLogoButton_pressed
 */
void Controls::on_c2CustomLogoButton_pressed()
{
    QString customLogo = QFileDialog::getOpenFileName(this,
                                        tr("Open Image"), ".",
                        tr("Image Files (*.png *.jpg *.bmp)"));

    qDebug() << "on_c2CustomFlagButton_pressed(): using customLogo string: " << customLogo;

    if (!customLogo.isEmpty()) {
        emit competitor2FlagChanged(customLogo);
    }
}

/**
 * @brief Controls::on_playPauseButton_pressed
 */
void Controls::on_playPauseButton_pressed()
{
    qDebug() << "----------------------------------------------";
    qDebug() << "Control:on_play_button_clicked() entry state: " <<
                "matchStarted: " << matchStarted << ", " <<
                "clockRunning: " << clockRunning << ", " <<
                "clockPaused: " << clockPaused;
    qDebug() << "----------------------------------------------";

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

    qDebug() << "----------------------------------------------";
    qDebug() << "Control:on_play_button_clicked() exit state: " <<
                "matchStarted: " << matchStarted << ", " <<
                "clockRunning: " << clockRunning << ", " <<
                "clockPaused: " << clockPaused;
    qDebug() << "----------------------------------------------";
}

/**
 * @brief Controls::on_resetButton_pressed
 */
void Controls::on_resetButton_pressed()
{
    qDebug() << "------------------------------------------";
    qDebug() << "Controls::on_resetButton_pressed() -- in.";
    qDebug() << "------------------------------------------";
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

    /* Give current index to our map to get total seconds for reset value,
     * because we to keep the same division and timings when reset is pressed rather
     * than requiring user to change those each time they hit reset.
     */
    totalTime = divisionIdxToTimeMap[ui->divisionComboBox->currentIndex()];

    qDebug() << "Contents of divisionIdxToTimeMap: " << divisionIdxToTimeMap;

    clockMinutes = totalTime / 60;
    clockSeconds = totalTime % 60;

    qDebug() << "Using totalTime: " << totalTime;
    qDebug() << "Using clockMinutes: " << clockMinutes;
    qDebug() << "Using clockSeconds: " << clockSeconds;

    // require updating display here to show reset values
    updateDisplay();

    // Update match states
    resetMatchStates();

    qDebug() << "------------------------------------------";
    qDebug() << "Controls::on_resetButton_pressed() -- out.";
    qDebug() << "------------------------------------------";
}

/**
 * @brief Controls::resetMatchStates
 */
void Controls::resetMatchStates()
{
    matchStarted = false;
    clockRunning = false;
    clockPaused = false;
    matchDone = false;
    emit matchReset();
}

/**
 * @brief Controls::onAboutToQuit
 *
 * Called when QApplication is about to quit main event loop.
 * Connects to QCoreApplication::aboutToQuit signal.
 * Last-second cleanup goes here. Cannot be emitted by user
 * and no user interaction possible in here.
 */
void Controls::onAboutToQuit()
{
    qDebug() << "onAboutToQuit called.";
}

/**
 * @brief Controls::on_c1Del3Button_pressed
 */
void Controls::on_c1Del3Button_pressed()
{
    QLabel *label = ui->c1PointsLabel;
    modify_points(label, -3);
    emit competitor1PointsChanged(-3);
}

/**
 * @brief Controls::on_c1Add4Button_pressed
 */
void Controls::on_c1Add4Button_pressed()
{
    QLabel *label = ui->c1PointsLabel;
    modify_points(label, 4);
    emit competitor1PointsChanged(4);

}

/**
 * @brief Controls::on_c1Del4Button_pressed
 */
void Controls::on_c1Del4Button_pressed()
{
    QLabel *label = ui->c1PointsLabel;
    modify_points(label, -4);
    emit competitor1PointsChanged(-4);
}

/**
 * @brief Controls::on_c1AddAButton_pressed
 */
void Controls::on_c1AddAButton_pressed()
{
    QLabel *label = ui->c1AdvantagesLabel;
    modify_points(label, 1);
    emit competitor1AdvantagesChanged(1);
}

/**
 * @brief Controls::on_c1DelAButton_pressed
 */
void Controls::on_c1DelAButton_pressed()
{
    QLabel *label = ui->c1AdvantagesLabel;
    modify_points(label, -1);
    emit competitor1AdvantagesChanged(-1);

}

/**
 * @brief Controls::on_c1AddPButton_pressed
 */
void Controls::on_c1AddPButton_pressed()
{
    QLabel *label = ui->c1PenaltiesLabel;
    modify_points(label, 1);
    emit competitor1PenaltiesChanged(1);
}

/**
 * @brief Controls::on_c1DelPButton_pressed
 */
void Controls::on_c1DelPButton_pressed()
{
    QLabel *label = ui->c1PenaltiesLabel;
    modify_points(label, -1);
    emit competitor1PenaltiesChanged(-1);
}

/**
 * @brief Controls::on_c2Add2Button_pressed
 */
void Controls::on_c2Add2Button_pressed()
{
   QLabel *label = ui->c2PointsLabel;
   modify_points(label, 2);
   emit competitor2PointsChanged(2);
}

/**
 * @brief Controls::on_c2Del2Button_pressed
 */
void Controls::on_c2Del2Button_pressed()
{
   QLabel *label = ui->c2PointsLabel;
   modify_points(label, -2);
   emit competitor2PointsChanged(-2);
}

/**
 * @brief Controls::on_c2Add3Button_pressed
 */
void Controls::on_c2Add3Button_pressed()
{
   QLabel *label = ui->c2PointsLabel;
   modify_points(label, 3);
   emit competitor2PointsChanged(3);
}

/**
 * @brief Controls::on_c2Del3Button_pressed
 */
void Controls::on_c2Del3Button_pressed()
{
   QLabel *label = ui->c2PointsLabel;
   modify_points(label, -3);
   emit competitor2PointsChanged(-3);
}

/**
 * @brief Controls::on_c2Add4Button_pressed
 */
void Controls::on_c2Add4Button_pressed()
{

   QLabel *label = ui->c2PointsLabel;
   modify_points(label, 4);
   emit competitor2PointsChanged(4);
}

/**
 * @brief Controls::on_c2Del4Button_pressed
 */
void Controls::on_c2Del4Button_pressed()
{
   QLabel *label = ui->c2PointsLabel;
   modify_points(label, -4);
   emit competitor2PointsChanged(-4);
}

/**
 * @brief Controls::on_c2AddAButton_pressed
 */
void Controls::on_c2AddAButton_pressed()
{
   QLabel *label = ui->c2AdvantagesLabel;
   modify_points(label, 1);
   emit competitor2AdvantagesChanged(1);
}

/**
 * @brief Controls::on_c2DelAButton_pressed
 */
void Controls::on_c2DelAButton_pressed()
{
   QLabel *label = ui->c2AdvantagesLabel;
   modify_points(label, -1);
   emit competitor2AdvantagesChanged(-1);
}

/**
 * @brief Controls::on_c2AddPButton_pressed
 */
void Controls::on_c2AddPButton_pressed()
{
   QLabel *label = ui->c2PenaltiesLabel;
   modify_points(label, 1);
   emit competitor2PenaltiesChanged(1);
}

/**
 * @brief Controls::on_c2DelPButton_pressed
 */
void Controls::on_c2DelPButton_pressed()
{
   QLabel *label = ui->c2PenaltiesLabel;
   modify_points(label, -1);
   emit competitor2PenaltiesChanged(-1);
}

/**
 * @brief Controls::on_c1LineEdit_textEdited
 * @param str QString
 */
void Controls::on_c1LineEdit_textEdited(const QString &str)
{
    ui->c1NameLabel->setText(str);
    emit competitor1NameChanged(str);
}

/**
 * @brief Controls::on_c2LineEdit_textEdited
 * @param str QString
 */
void Controls::on_c2LineEdit_textEdited(const QString &str)
{
   ui->c2NameLabel->setText(str);
   emit competitor2NameChanged(str);
}

/**
 * @brief Controls::on_testSoundButton_pressed
 */
void Controls::on_testSoundButton_pressed()
{
    playSound();
}

/**
 * @brief Controls::on_c1FlagComboBox_currentIndexChanged
 * @param flag QString
 *
 * Emits a QString representing the path to the big flag version of the
 * small flag selected. This is so we can load small version of flag for
 * speed at startup into the dropdown, and load big version only when
 * needed, i.e., right here.
 */
void Controls::on_c1FlagComboBox_currentIndexChanged(const QString &flag)
{
    QString bigflag = ":/flags/" + flag;
    emit competitor1FlagChanged(bigflag);
}

/**
 * @brief Controls::on_c2FlagComboBox_currentIndexChanged
 * @param flag QString
 */
void Controls::on_c2FlagComboBox_currentIndexChanged(const QString &flag)
{
    QString bigflag = ":/flags/" + flag;
    emit competitor2FlagChanged(bigflag);
}
