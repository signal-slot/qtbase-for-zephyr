// Tier 3 GUI smoke test on MIMXRT1170-EVKB + RK055HDMIPI4MA0.
// Animates a rotating rounded rect through the qzephyr QPA plugin which
// flushes pixels via Zephyr's display_write() (see qzephyr_display_zephyr.cpp).

#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <QtGui/QBackingStore>
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtGui/QFontDatabase>
#include <QtCore/QTimer>
#include <QtCore/QByteArray>
#include <QtGui/QScreen>

extern "C" unsigned char g_embedded_roboto_ttf[];
extern "C" unsigned int g_embedded_roboto_ttf_len;

#include <zephyr/kernel.h>

class RasterWindow : public QWindow
{
public:
    RasterWindow()
    {
        setTitle("Qt6 on RT1170 (Tier 3)");
        if (QScreen *s = QGuiApplication::primaryScreen())
            resize(s->geometry().size());
        else
            resize(720, 1280);

        m_backingStore.reset(new QBackingStore(this));

        QTimer *t = new QTimer(this);
        QObject::connect(t, &QTimer::timeout, this, [this]() {
            m_angle += 4.0;
            ++m_frame;
            if (m_frame % 60 == 0)
                printk("[qt-gui] frame=%lu angle=%.0f\n",
                       (unsigned long)m_frame, m_angle);
            renderNow();
        });
        t->start(16);
        show();
    }

protected:
    bool event(QEvent *e) override
    {
        switch (e->type()) {
        case QEvent::UpdateRequest:
            renderNow();
            return true;
        case QEvent::MouseButtonPress: {
            auto *me = static_cast<QMouseEvent *>(e);
            m_pressed = true;
            m_lastTouch = me->pos();
            printk("[qt-gui] touch press at (%d, %d)\n", me->pos().x(), me->pos().y());
            return true;
        }
        case QEvent::MouseButtonRelease: {
            auto *me = static_cast<QMouseEvent *>(e);
            m_pressed = false;
            printk("[qt-gui] touch release at (%d, %d)\n", me->pos().x(), me->pos().y());
            return true;
        }
        case QEvent::MouseMove: {
            auto *me = static_cast<QMouseEvent *>(e);
            m_lastTouch = me->pos();
            return true;
        }
        default:
            return QWindow::event(e);
        }
    }

    void exposeEvent(QExposeEvent *) override
    {
        if (isExposed())
            renderNow();
    }

private:
    void renderNow()
    {
        // QZephyrWindow currently never delivers a proper expose event so
        // isExposed() stays false even when the panel is visible.  Skip
        // the guard for now; tracked as a Tier 3.5 polish task.
        if (m_backingStore->size() != size())
            m_backingStore->resize(size());

        QRegion r(QRect(QPoint(0, 0), size()));
        m_backingStore->beginPaint(r);
        QPaintDevice *dev = m_backingStore->paintDevice();
        QPainter p(dev);
        p.fillRect(0, 0, width(), height(), QColor(30, 30, 60));
        p.setRenderHint(QPainter::Antialiasing);
        p.translate(width() / 2, height() / 2);
        p.rotate(m_angle);
        p.setBrush(m_pressed ? QColor(255, 100, 0) : QColor(0, 170, 255));
        p.setPen(Qt::NoPen);
        const int s = qMin(width(), height()) / 3;
        p.drawRoundedRect(QRect(-s, -s, 2 * s, 2 * s), 20, 20);

        // Text -- Qt6 + FreeType on Cortex-M7 demo.  Painted in screen
        // coordinates (resetTransform clears the rotate above).
        p.resetTransform();
        p.setPen(Qt::white);
        QFont f = p.font();
        f.setPixelSize(48);
        f.setBold(true);
        p.setFont(f);
        p.drawText(QRect(0, 40, width(), 80), Qt::AlignHCenter,
                   QStringLiteral("Qt 6.11 on Zephyr / RT1170"));
        f.setPixelSize(24);
        f.setBold(false);
        p.setFont(f);
        p.drawText(QRect(0, height() - 80, width(), 40), Qt::AlignHCenter,
                   QString::fromLatin1("frame %1   angle %2°")
                       .arg(m_frame).arg(int(m_angle) % 360));

        if (m_pressed || !m_lastTouch.isNull()) {
            p.setBrush(QColor(255, 255, 0));
            p.setPen(Qt::NoPen);
            p.drawEllipse(m_lastTouch, 20, 20);
        }
        p.end();
        m_backingStore->endPaint();
        m_backingStore->flush(r);
    }

    std::unique_ptr<QBackingStore> m_backingStore;
    qreal m_angle = 0;
    unsigned long m_frame = 0;
    bool m_pressed = false;
    QPoint m_lastTouch;
};

int main(int argc, char **argv)
{
    printk("[qt-gui] pre-Qt main(); arg count = %d\n", argc);
    qputenv("QT_QPA_PLATFORM", QByteArray("zephyr"));

    QGuiApplication app(argc, argv);
    printk("[qt-gui] QGuiApplication constructed\n");

    // Register the embedded Roboto so QPainter::drawText has glyphs.
    // Without this, FreeType FontDatabase returns no font and drawText
    // renders tofu boxes.
    const QByteArray fontData(reinterpret_cast<const char *>(g_embedded_roboto_ttf),
                              int(g_embedded_roboto_ttf_len));
    const int fontId = QFontDatabase::addApplicationFontFromData(fontData);
    printk("[qt-gui] addApplicationFontFromData -> id=%d (-1 means failed)\n", fontId);
    if (fontId >= 0) {
        const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        if (!families.isEmpty())
            QGuiApplication::setFont(QFont(families.first()));
    }

    RasterWindow w;
    printk("[qt-gui] RasterWindow shown; entering exec()\n");

    const int rc = app.exec();
    printk("[qt-gui] app.exec() returned %d\n", rc);
    return rc;
}
