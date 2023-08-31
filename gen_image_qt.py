import sys
from PyQt5.QtWidgets import QApplication, QWidget
from PyQt5.QtGui import QIcon, QPainter, QPen, QPixmap, QPainterPath, QColor
from PyQt5.QtCore import Qt, QRectF, QPoint, QPointF, QVariantAnimation


ROD_LENGTH = 2  # meters
NUM_LEDS = 170
IMAGE_HEIGHT = 70

ZOOM = 1
CANVAS_WIDTH = NUM_LEDS * 1 * ZOOM
CANVAS_HEIGHT = IMAGE_HEIGHT * 1 * ZOOM


class MyWindow(QWidget):
    def __init__(self, width, height):
        super().__init__()
        self.resize(width, height)
        self.setWindowTitle('My PyQt Window')

    def paintEvent(self, event):
        self.draw_bg()
        self.draw_eye_iris(self.height())
        self.draw_pupil()

    def draw_eye_iris(self, diameter):
        painter = QPainter(self)
        painter.setRenderHints(painter.Antialiasing)
        painter.setPen(QPen(Qt.white, self.w, Qt.SolidLine))

        # if self.relative:
        # diameter *= min(self.width(), self.height())

        rect = QRectF(0, 0, diameter, diameter)
        rect.moveCenter(QRectF(self.rect()).center())

        path = QPainterPath()
        path.addEllipse(rect)
        painter.drawPath(path)

    def draw_pupil(self):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)
        painter.setBrush(Qt.white)
        rect = QRectF(0, 0, 80, self.height()*0.8)
        rect.moveCenter(QRectF(self.rect()).center())

        path = QPainterPath()
        path.addEllipse(rect)
        painter.drawPath(path)

    def draw_bg(self):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)
        painter.setPen(QColor(255, 0, 0))
        painter.setBrush(QColor(0, 0, 0))
        rect = QRectF(0, 0, self.width(), self.height())
        rect.moveCenter(QRectF(self.rect()).center())
        painter.drawRect(rect)


app = QApplication(sys.argv)


window = MyWindow(CANVAS_WIDTH, CANVAS_HEIGHT)
window.show()

sys.exit(app.exec_())
