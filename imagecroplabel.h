#ifndef IMAGECROPLABEL_H
#define IMAGECROPLABEL_H
#include <QWidget>
#include <QLabel>
#include <QRect>
#include <QPaintEvent>
#include <QHoverEvent>
#include <QMouseEvent>

class ImageCropLabel : public QLabel
{
    Q_OBJECT
public:
    explicit ImageCropLabel(QWidget *parent = nullptr);

    void loadImage(QImage image);

public slots:
    void scaleImage(int value);
    //void exportToFile(QString filename);

signals:

private:
    QRect selectRect;
    QImage unscaledImage;

    // Offset to keep the image in the center
    QPoint imageOffset;

    // enum to indicate which part of the QRect
    // BEGAIN and END used as helper for iteration
    enum RectPart
    {
        BEGAIN,
        Body,
        RightEdge,
        LeftEdge,
        TopEdge,
        BottomEdge,
        TRCorner,
        BRCorner,
        TLCorner,
        BLCorner,
        END
    };

    bool draging;
    QPoint dragBodyOffset;
    // Map to store which RectPart is selected for draging
    QMap<RectPart, bool> selectedRectPart;

protected:
    void paintEvent(QPaintEvent *event) override;

    bool event(QEvent *e) override;
    void setDragCursor(QPoint pos);
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;

};

#endif // IMAGECROPLABEL_H
