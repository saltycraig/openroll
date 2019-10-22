#include "scoreboard.h"
#include "ui_scoreboard.h"
#ifdef LOGGER
    #include "logger.h"
#endif
#include <QDebug>

/**
 * @brief Scoreboard::closeEvent
 * @param event QCloseEvent
 */
void Scoreboard::closeEvent(QCloseEvent *event)
{
    qDebug() << "Close event caught on scoreboard";
    event->accept();
}

/**
 * @brief Scoreboard::Scoreboard
 * @param parent QWidget
 * @param f Qt::WindowFlags
 */
Scoreboard::Scoreboard(QWidget *parent, Qt::WindowFlags f) :
    QWidget(parent),
    ui(new Ui::Scoreboard)
{
    ui->setupUi(this);
    setWindowFlags(f);
}

/**
 * @brief Scoreboard::~Scoreboard
 */
Scoreboard::~Scoreboard()
{
    delete ui;
}

/**
 * @brief Scoreboard::setCompetitor1Points
 * @param value int
 */
void Scoreboard::setCompetitor1Points(int value)
{
    int oldValue = ui->comp1_points->text().toInt();
    int newValue = oldValue + value < 0 ? 0 : oldValue + value;
    ui->comp1_points->setText(QString::number(newValue));
}

/**
 * @brief Scoreboard::setCompetitor1Advantages
 * @param value int
 */
void Scoreboard::setCompetitor1Advantages(int value)
{
    int oldValue = ui->comp1_advantages->text().toInt();
    int newValue = oldValue + value < 0 ? 0 : oldValue + value;
    ui->comp1_advantages->setText(QString::number(newValue));
}

/**
 * @brief Scoreboard::setCompetitor1Penalties
 * @param value int
 */
void Scoreboard::setCompetitor1Penalties(int value)
{
    int oldValue = ui->comp1_penalties->text().toInt();
    int newValue = oldValue + value < 0 ? 0 : oldValue + value;
    ui->comp1_penalties->setText(QString::number(newValue));
}

/**
 * @brief Scoreboard::setCompetitor2Points
 * @param value int
 */
void Scoreboard::setCompetitor2Points(int value)
{
    int oldValue = ui->comp2_points->text().toInt();
    int newValue = oldValue + value < 0 ? 0 : oldValue + value;
    ui->comp2_points->setText(QString::number(newValue));
}

/**
 * @brief Scoreboard::setCompetitor2Advantages
 * @param value int
 */
void Scoreboard::setCompetitor2Advantages(int value)
{
    int oldValue = ui->comp2_advantages->text().toInt();
    int newValue = oldValue + value < 0 ? 0 : oldValue + value;
    ui->comp2_advantages->setText(QString::number(newValue));
}

/**
 * @brief Scoreboard::setCompetitor2Penalties
 * @param value int
 */
void Scoreboard::setCompetitor2Penalties(int value)
{
    int oldValue = ui->comp2_penalties->text().toInt();
    int newValue = oldValue + value < 0 ? 0 : oldValue + value;
    ui->comp2_penalties->setText(QString::number(newValue));
}

/**
 * @brief Scoreboard::setCompetitor1Name
 * @param name QString
 */
void Scoreboard::setCompetitor1Name(const QString& name)
{
    ui->comp1_name->setText(name);
}

/**
 * @brief Scoreboard::setCompetitor2Name
 * @param name QString
 */
void Scoreboard::setCompetitor2Name(const QString& name)
{
    ui->comp2_name->setText(name);
}

/**
 * @brief Scoreboard::setTimeLabel
 * @param newtime QString
 */
void Scoreboard::setTimeLabel(const QString& newtime)
{
    ui->timer_display->setText(newtime);
}

/**
 * @brief Scoreboard::setDivisionLabel
 * @param division QString
 */
void Scoreboard::setDivisionLabel(const QString& division)
{
    ui->divisionLabel->setText(division);
}

/**
 * @brief Scoreboard::setBeltLabel
 * @param newbelt QString
 */
void Scoreboard::setBeltLabel(const QString& newbelt)
{
    ui->beltLabel->setText(newbelt);
}

/**
 * @brief Scoreboard::setLogo
 * @param logo QString
 */
void Scoreboard::setLogo(const QString& logo)
{
    qDebug() << "Scoreboard given this logo to try to load: " << logo;
    QPixmap img = QPixmap(logo);
    ui->user_logo->setPixmap(img);
}

/**
 * @brief Scoreboard::resetScores
 */
void Scoreboard::resetScores()
{
    ui->comp1_points->setText("0");
    ui->comp1_advantages->setText("0");
    ui->comp1_penalties->setText("0");
    ui->comp1_name->setText("Competitor 1");

    ui->comp2_points->setText("0");
    ui->comp2_advantages->setText("0");
    ui->comp2_penalties->setText("0");
    ui->comp2_name->setText("Competitor 2");
}

/**
 * @brief Scoreboard::setC1Flag
 * @param flagpath QString
 */
void Scoreboard::setC1Flag(const QString& flagpath)
{
    ui->c1FlagLabel->setPixmap(flagpath);
}

/**
 * @brief Scoreboard::setC2Flag
 * @param flagpath QString
 */
void Scoreboard::setC2Flag(const QString& flagpath)
{
   ui->c2FlagLabel->setPixmap(flagpath);
}
