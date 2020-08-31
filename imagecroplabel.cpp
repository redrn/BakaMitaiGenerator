#include "imagecroplabel.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QPen>
#include <QColorSpace>
#include <QGuiApplication>
#include <QMap>
#include <algorithm>

ImageCropLabel::ImageCropLabel(QWidget *parent) : QLabel(parent)
{
    // Initialize Image Viewer
    this->setBackgroundRole(QPalette::Base);
    //ui->sourceImageViewLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    //this->setScaledContents(true);
    this->setAttribute(Qt::WA_Hover);

    draging = false;

    // Kind of annoying that c++ does not provide direct iteration over enum
    for(int partInt = RectPart::BEGAIN; partInt != RectPart::END; partInt++)
    {
        RectPart part = static_cast<RectPart>(partInt);
        selectedRectPart.insert(part, false);
    }
}

void ImageCropLabel::loadImage(QImage image)
{
    // Scale pixmap to size of this label, without changing aspect ratio
    unscaledImage = image.scaled(this->width(), this->height(),
                                 Qt::KeepAspectRatio, Qt::SmoothTransformation);
    unscaledImage = unscaledImage.convertToFormat(QImage::Format_RGB32);
    unscaledImage.convertToColorSpace(QColorSpace::SRgb);
    this->setPixmap(QPixmap::fromImage(unscaledImage));

    // Create default selection rect
    // Use 256 as default size unless pixmap is smaller, positioned in the center
    int sideLength = 256;
    if(sideLength > unscaledImage.height() || sideLength > unscaledImage.width())
        sideLength = unscaledImage.height() <= unscaledImage.width() ?
                    unscaledImage.height() : unscaledImage.width();
    selectRect = QRect(0, 0, sideLength, sideLength);
    selectRect.moveCenter(unscaledImage.rect().center());

    // Darken image enclosed by selectionRect
    darkenSelection();
}

// Scale image between factor [1, 199]
void ImageCropLabel::scaleImage(int value)
{
    if(unscaledImage.isNull()) return;

    // Normalize scale factor to between 1% and 199%
    // X' = (X - Xmin) / (Xmax - Xmin)
    double factor = (double)value / 100.0;
    qDebug()<<factor;
    // Set pixmap
    this->setPixmap(QPixmap::fromImage(unscaledImage.scaled(unscaledImage.size() * factor,
                                          Qt::KeepAspectRatio,
                                          Qt::SmoothTransformation)));
}

void ImageCropLabel::darkenSelection()
{

}

void ImageCropLabel::paintEvent(QPaintEvent *event)
{
    QLabel::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen pen(Qt::white, 3);
    painter.setPen(pen);

    // Draw rect
    painter.drawRect(selectRect);

    // Draw four Corner
    pen.setWidth(10);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    // NOTE: refer to qt doc, right() and bottom() are 1 point off from real pos
    painter.drawPoint(selectRect.topLeft());
    painter.drawPoint(selectRect.topRight());
    painter.drawPoint(selectRect.bottomLeft());
    painter.drawPoint(selectRect.bottomRight());
}

// Handle hover event and change cursor accordingly
bool ImageCropLabel::event(QEvent *e)
{
    switch (e->type())
    {
    case QEvent::HoverMove:
        setDragCursor(static_cast<QHoverEvent*>(e)->pos());
        return true;

    default:
        break;
    }

    return QLabel::event(e);
}

// Change cursor based on cursor position
void ImageCropLabel::setDragCursor(QPoint pos)
{
    // TODO: change from absolute tolerance to relative tolerance
    //       ie. keep same visual size regardless of actual screen size

    /* If within range of rect corner or line, change cursor,
       and set map to indicate selection */

    // Cursor and selection should not change during draging
    if(draging) return;

    // Helper lambda to set only one value in QMap to true and others to false
    auto setUniqueTrue = [&map = selectedRectPart](RectPart part)
    {
        for(auto itr = map.begin(); itr != map.end(); ++itr)
        {
            itr.value() = false;
        }
        map[part] = true;
    };

    // Check if tolerance rect for each line and corner contains pos
    int tolerance = 4;

    QPoint offset(-tolerance, tolerance);
    // Right edge
    if(QRect(selectRect.topRight() + offset,
             selectRect.bottomRight() - offset).contains(pos))
    {
        QGuiApplication::setOverrideCursor(QCursor(Qt::SizeHorCursor));
        setUniqueTrue(RectPart::RightEdge);
        return;
    }
    // Left edge
    if(QRect(selectRect.topLeft() + offset,
             selectRect.bottomLeft() - offset).contains(pos))
    {
        QGuiApplication::setOverrideCursor(QCursor(Qt::SizeHorCursor));
        setUniqueTrue(RectPart::LeftEdge);
        return;
    }

    offset = QPoint(tolerance, -tolerance);
    // Top edge
    if(QRect(selectRect.topLeft() + offset,
             selectRect.topRight() - offset).contains(pos))
    {
        QGuiApplication::setOverrideCursor(QCursor(Qt::SizeVerCursor));
        setUniqueTrue(RectPart::TopEdge);
        return;
    }
    // Bottom edge
    if(QRect(selectRect.bottomLeft() + offset,
             selectRect.bottomRight() - offset).contains(pos))
    {
        QGuiApplication::setOverrideCursor(QCursor(Qt::SizeVerCursor));
        setUniqueTrue(RectPart::BottomEdge);
        return;
    }

    // Four corners
    // lambda expression used to simplfy code
    // cursorSet is used as workaround that
    // I cannot terminate (return) this function after cursor is set
    // since calling return in lambda will only terminate the lambda fuction
    bool cursorSet = false;
    // tolerance is doulbed because the corner botton themselves are bigger
    tolerance *= 2;
    offset = QPoint(-tolerance, -tolerance);
    auto setCornerCursor = [&](QPoint corner, Qt::CursorShape shape, RectPart part)
    {
        if(QRect(corner + offset, corner - offset).contains(pos))
        {
            QGuiApplication::setOverrideCursor(QCursor(shape));
            setUniqueTrue(part);
            cursorSet = true;
        }
    };
    setCornerCursor(selectRect.topRight(), Qt::SizeBDiagCursor, RectPart::TRCorner);
    setCornerCursor(selectRect.bottomLeft(), Qt::SizeBDiagCursor, RectPart::BLCorner);
    setCornerCursor(selectRect.topLeft(), Qt::SizeFDiagCursor, RectPart::TLCorner);
    setCornerCursor(selectRect.bottomRight(), Qt::SizeFDiagCursor, RectPart::BRCorner);

    // Body of the selectRect
    if(selectRect.contains(pos, true))
    {
         QGuiApplication::setOverrideCursor(QCursor(Qt::SizeAllCursor));
         setUniqueTrue(RectPart::Body);
         return;
    }

    /* if not, restore cursor override */
    if(!cursorSet)
    {
        QGuiApplication::restoreOverrideCursor();
        // reset all to false
        for(auto itr = selectedRectPart.begin(); itr != selectedRectPart.end(); ++itr)
        {
            itr.value() = false;
        }
        //attempted to shorten the for loop aboce into one line, failed
        //std::for_each(selectedRectPart.begin(), selectedRectPart.end(), [](auto itr){itr.value() = false;});
    }
}

void ImageCropLabel::mousePressEvent(QMouseEvent *ev)
{
    draging = true;
}

void ImageCropLabel::mouseReleaseEvent(QMouseEvent *ev)
{
    draging = false;
}

void ImageCropLabel::mouseMoveEvent(QMouseEvent *ev)
{
    if(draging)
    {
        RectPart part = RectPart::BEGAIN;
        for(auto itr = selectedRectPart.begin(); itr != selectedRectPart.end(); ++itr)
        {
            if(itr.value() == true) part = itr.key();
        }

        switch(part)
        {
        case RectPart::TopEdge:
            selectRect.setTop(ev->pos().y());
            break;
        case RectPart::BottomEdge:
            selectRect.setBottom(ev->pos().y());
            break;
        case RectPart::RightEdge:
            selectRect.setRight(ev->pos().x());
            break;
        case RectPart::LeftEdge:
            selectRect.setLeft(ev->pos().x());
            break;
        case RectPart::TRCorner:
            selectRect.setTopRight(ev->pos());
            break;
        case RectPart::BRCorner:
            selectRect.setBottomRight(ev->pos());
            break;
        case RectPart::TLCorner:
            selectRect.setTopLeft(ev->pos());
            break;
        case RectPart::BLCorner:
            selectRect.setBottomLeft(ev->pos());
            break;
        default:
            break;
        }

        update(); // necessary as this will call paintEvent
    }
}
