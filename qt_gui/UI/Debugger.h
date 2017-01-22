#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <QWidget>
#include <QTimer>
#include <emucore.h>
namespace Ui {
class Debugger;
}

class Debugger : public QWidget
{
    Q_OBJECT

private:
	Ui::Debugger *ui;
	QTimer * clock;
	QTimer * tlog;
	QTimer * tips;
	size_t log_lastsz = 0;

public:
	Ymgyrch::Core * emu;

    explicit Debugger(QWidget *parent = 0);
    ~Debugger();

public slots:
	void DoUpdate();
	void UpdateLog();
	void OnSetUpdateSpeed(int);
	void CalculateIPS();
};

#endif // DEBUGGER_H