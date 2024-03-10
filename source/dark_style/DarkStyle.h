#ifndef DARKSTYLE_HPP
#define DARKSTYLE_HPP

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QProxyStyle>
#include <QStyleFactory>

#include "WinDark.h"


namespace dark_style
{
    extern bool shouldUseDarkStyle();
    extern void useDarkStyleOnApplication();
    extern void useDarkStyleOnWidget(QWidget *target);
}

class DarkStyle : public QProxyStyle {
  Q_OBJECT

 public:
  DarkStyle();
  explicit DarkStyle(QStyle *style);

  QStyle *baseStyle() const;

  void polish(QPalette &palette) override;
  void polish(QApplication *app) override;

 private:
  QStyle *styleBase(QStyle *style = Q_NULLPTR) const;
};

#endif  // DARKSTYLE_HPP
