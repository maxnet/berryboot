#include "twoiconsdelegate.h"
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QModelIndex>

TwoIconsDelegate::TwoIconsDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

void TwoIconsDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    QVariant v = index.data(SecondIconRole);
    if (!v.isNull() && v.canConvert<QIcon>())
    {
        QIcon icon = v.value<QIcon>();
        QSize size = icon.availableSizes().first();

        painter->drawPixmap(option.rect.right()-size.width()-5, option.rect.top()+5, icon.pixmap(size));
    }
}
