#include "controls.h"
#include <QApplication>
#ifdef LOGGER
  #include "logger.h"
#endif
#include <QDebug>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  Controls controls;

  const Qt::WindowFlags flags(Qt::Window | Qt::WindowStaysOnTopHint);
  controls.setWindowFlags(flags);
  QObject::connect(&app, &QCoreApplication::aboutToQuit, &controls,
                   &Controls::onAboutToQuit);
  controls.show();

#ifdef LOGGER
  qInfo() << "Using log file named: " << logger::filename;
  logger::attach();
#endif

  return app.exec();
}

// DEV: Load your own sound file
// DEV: Use millisecond precision, e.g. 00:00:000 on clock and displays.
/* BUG: Fix naive QTimer.
 * Currently when you pause the timer and restart the match with play button again, a new timer is
 * created to fire again in 1s. Really it should just continue from time remaining on the previous timer,
 * otherwise user can just keep hitting play/pause every .5 seconds and the screen would never update.
 */
