#ifndef TWOICONSDELEGATE_H
#define TWOICONSDELEGATE_H

#include <QStyledItemDelegate>

#define SecondIconRole  (Qt::UserRole+10)

class TwoIconsDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit TwoIconsDelegate(QObject *parent = 0);
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
signals:

public slots:

};

#endif // TWOICONSDELEGATE_H
