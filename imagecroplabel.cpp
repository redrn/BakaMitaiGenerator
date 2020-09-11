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
#include <QDir>

ImageCropLabel::ImageCropLabel(QWidget *parent) : QLabel(parent)
{
    // Initialize Image Viewer
    this->setBackgroundRole(QPalette::Base);
    //this->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    //this->setScaledContents(true);
    this->setAttribute(Qt::WA_Hover);
    this->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

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

    // Center the image
    imageOffset = this->rect().center() - unscaledImage.rect().center();
    unscaledImage.setOffset(imageOffset);

    this->setPixmap(QPixmap::fromImage(unscaledImage));

    // Create default selection rect
    // Use 256 as default size unless pixmap is smaller, positioned in the center
    int sideLength = 256;
    if(sideLength > unscaledImage.height() || sideLength > unscaledImage.width())
        sideLength = unscaledImage.height() <= unscaledImage.width() ?
                    unscaledImage.height() : unscaledImage.width();
    selectRect = QRect(0, 0, sideLength, sideLength);
    selectRect.moveCenter(unscaledImage.rect().center() + imageOffset);
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

void ImageCropLabel::paintEvent(QPaintEvent *event)
{
    // Call parent's paintEvent
    QLabel::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    /* Darken unselected area */
    // Create rects to be filled
    QVector<QRect> rects{};
    QRect imageRect = unscaledImage.rect();
    // top rect
    if(selectRect.top() - (imageRect.top() + imageOffset.y()) >= 0)
    {
        //qDebug() << "drawing top";
        rects.push_back(QRect(imageRect.topLeft() + imageOffset,
                              QPoint(imageRect.right() + imageOffset.x(),
                                     selectRect.topRight().y())));
    }
    // bottom rect
    if(imageRect.bottom() + imageOffset.y() - selectRect.bottom() >= 0)
    {
        //qDebug() << "drawing bottom";
        rects.push_back(QRect(QPoint(imageOffset.x(), selectRect.bottomLeft().y()),
                              imageRect.bottomRight() + imageOffset));
    }
    // left rect
    if(selectRect.left() - (imageRect.left() + imageOffset.x()) >= 0)
    {
        //qDebug() << "drawing left";
        // NOTE: top() + 1 and QPoint(0, 1) are to eliminate overlap between rects,
        // caused by inaccurate bottom() and right() in QRect
        rects.push_back(QRect(QPoint(imageRect.left() + imageOffset.x(), selectRect.top() + 1),
                              selectRect.bottomLeft() - QPoint(0, 1)));
    }
    // right rect
    if(imageRect.right() + imageOffset.x() - selectRect.right() >= 0)
    {
        // NOTE: + QPoint(0, 1) and -1 also used to eliminate overlap
        //qDebug() << "drawing right";
        rects.push_back(QRect(selectRect.topRight() + QPoint(0, 1),
                              QPoint(imageRect.right() + imageOffset.x(), selectRect.bottom() - 1)));
    }

    // Fill all rects
    std::for_each(rects.begin(), rects.end(), [&](auto r)
    {
        painter.fillRect(r, QColor(0, 0, 0, 0.7 * 255));
    });

    /* Draw selection rect */
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
        this->setCursor(QCursor(Qt::SizeHorCursor));
        setUniqueTrue(RectPart::RightEdge);
        return;
    }
    // Left edge
    if(QRect(selectRect.topLeft() + offset,
             selectRect.bottomLeft() - offset).contains(pos))
    {
        this->setCursor(QCursor(Qt::SizeHorCursor));
        setUniqueTrue(RectPart::LeftEdge);
        return;
    }

    offset = QPoint(tolerance, -tolerance);
    // Top edge
    if(QRect(selectRect.topLeft() + offset,
             selectRect.topRight() - offset).contains(pos))
    {
        this->setCursor(QCursor(Qt::SizeVerCursor));
        setUniqueTrue(RectPart::TopEdge);
        return;
    }
    // Bottom edge
    if(QRect(selectRect.bottomLeft() + offset,
             selectRect.bottomRight() - offset).contains(pos))
    {
        this->setCursor(QCursor(Qt::SizeVerCursor));
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
            this->setCursor(QCursor(shape));
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
         this->setCursor(QCursor(Qt::SizeAllCursor));
         setUniqueTrue(RectPart::Body);
         return;
    }

    /* if not, restore cursor override */
    if(!cursorSet)
    {
        this->unsetCursor();
        // reset all to false
        for(auto itr = selectedRectPart.begin(); itr != selectedRectPart.end(); ++itr)
        {
            itr.value() = false;
        }
        return;
        //attempted to shorten the for loop aboce into one line, failed
        //std::for_each(selectedRectPart.begin(), selectedRectPart.end(), [](auto itr){itr.value() = false;});
    }
}


// Mouse events to enable dragging of selectRect
void ImageCropLabel::mousePressEvent(QMouseEvent *ev)
{
    draging = true;
    // Offset to keep relative position between cursor and rect center
    dragBodyOffset = selectRect.topLeft() - ev->pos();
}

void ImageCropLabel::mouseReleaseEvent(QMouseEvent *ev)
{
    draging = false;
}

void ImageCropLabel::mouseMoveEvent(QMouseEvent *ev)
{
    // FIXME: Prevent selectRect from being dragged outside of image rect
    if(draging)
    {
        // Find the selected part
        RectPart part = RectPart::BEGAIN;
        for(auto itr = selectedRectPart.begin(); itr != selectedRectPart.end(); ++itr)
        {
            if(itr.value() == true) part = itr.key();
        }


        /* Move selectRect
          and prevent it from moving outside of boundary */
        int minSize = 10;
        int targetPos;
        QPoint targetPoint;
        QRect imageRect = unscaledImage.rect();

        // Helper lambda to keep target pos within min and max
        auto keepMinMax = [&](int input, int min, int max)
        {
            // Return input if it's within (min, max)
            // Return min/max otherwise
            return input < max ? (input > min ? input : min) : (max);
        };

        switch(part)
        {
        case RectPart::TopEdge:
            targetPos = keepMinMax(ev->y(),
                                   imageRect.top() + imageOffset.y(),
                                   selectRect.bottom() + 1 - minSize);
            selectRect.setTop(targetPos);
            break;
        case RectPart::BottomEdge:
            targetPos = keepMinMax(ev->y(),
                                   selectRect.top() + minSize,
                                   imageRect.bottom() + 1 + imageOffset.y());
            selectRect.setBottom(targetPos);
            break;
        case RectPart::RightEdge:
            targetPos = keepMinMax(ev->x(),
                                   selectRect.left() + minSize,
                                   imageRect.right() + 1 + imageOffset.x());
            selectRect.setRight(targetPos);
            break;
        case RectPart::LeftEdge:
            targetPos = keepMinMax(ev->x(),
                                   imageRect.left() + imageOffset.x(),
                                   selectRect.right() + 1 - minSize);
            selectRect.setLeft(targetPos);
            break;
        case RectPart::TRCorner:
            targetPoint = QPoint(keepMinMax(ev->x(),
                                            selectRect.left() + minSize,
                                            imageRect.right() + 1 + imageOffset.x()),
                                 keepMinMax(ev->y(),
                                            imageRect.top() + imageOffset.y(),
                                            selectRect.bottom() + 1 - minSize));
            selectRect.setTopRight(targetPoint);
            break;
        case RectPart::BRCorner:
            targetPoint = QPoint(keepMinMax(ev->x(),
                                            selectRect.left() + minSize,
                                            imageRect.right() + 1 + imageOffset.x()),
                                 keepMinMax(ev->y(),
                                            selectRect.top() + minSize,
                                            imageRect.bottom() + 1 + imageOffset.y()));
            selectRect.setBottomRight(targetPoint);
            break;
        case RectPart::TLCorner:
            targetPoint = QPoint(keepMinMax(ev->x(),
                                            imageRect.left() + imageOffset.x(),
                                            selectRect.right() + 1 - minSize),
                                 keepMinMax(ev->y(),
                                            imageRect.top() + imageOffset.y(),
                                            selectRect.bottom() + 1 - minSize));
            selectRect.setTopLeft(targetPoint);
            break;
        case RectPart::BLCorner:
            targetPoint = QPoint(keepMinMax(ev->x(),
                                            imageRect.left() + imageOffset.x(),
                                            selectRect.right() + 1 - minSize),
                                 keepMinMax(ev->y(),
                                            selectRect.top() + minSize,
                                            imageRect.bottom() + 1 + imageOffset.y()));
            selectRect.setBottomLeft(targetPoint);
            break;
        case RectPart::Body:
            targetPoint = QPoint(keepMinMax(ev->x() + dragBodyOffset.x(),
                                            imageRect.left() + imageOffset.x(),
                                            imageRect.right() + 1 + imageOffset.x() - selectRect.width()),
                                 keepMinMax(ev->y() + dragBodyOffset.y(),
                                            imageRect.top() + imageOffset.y(),
                                            imageRect.bottom() + 1 + imageOffset.y() - selectRect.height()));
            selectRect.moveTopLeft(targetPoint);
            break;
        default:
            break;
        }

        update(); // necessary as this will call paintEvent
    }
}

QTemporaryFile *ImageCropLabel::extractSelected()
{
    QTemporaryFile *testFile = new QTemporaryFile(QCoreApplication::applicationName() +
                                                  ".XXXXXX" + ".png",
                                                  this);
    if(testFile->open())
    {
        unscaledImage.copy(selectRect.translated(-imageOffset)).save(testFile);
    }
    qDebug() << testFile->fileName();
    return testFile;
}
