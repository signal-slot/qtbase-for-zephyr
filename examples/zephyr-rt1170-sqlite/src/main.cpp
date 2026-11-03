#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include <zephyr/kernel.h>

int main(int /*argc_zephyr*/, char * /*argv_zephyr*/[])
{
    printk("\n[sqlite] === boot ===\n");

    static char arg0[] = "sqlite";
    static char *argv[] = { arg0, nullptr };
    int argc = 1;

    qputenv("QT_QPA_PLATFORM", "zephyr");
    printk("[sqlite] creating QApplication...\n");
    QApplication app(argc, argv);
    printk("[sqlite] QApplication created\n");

    QWidget w;
    auto *layout = new QVBoxLayout(&w);
    auto *titleLabel = new QLabel("SQLite on Zephyr");
    titleLabel->setStyleSheet("font-size: 32px; font-weight: bold;");
    layout->addWidget(titleLabel);

    auto *resultLabel = new QLabel;
    resultLabel->setStyleSheet("font-size: 20px;");
    resultLabel->setWordWrap(true);
    layout->addWidget(resultLabel);

    w.showFullScreen();

    QString result;

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");

    if (!db.open()) {
        result = QString("FAIL: %1").arg(db.lastError().text());
        printk("[sqlite] open failed: %s\n", qPrintable(db.lastError().text()));
    } else {
        printk("[sqlite] in-memory database opened\n");

        QSqlQuery q;
        q.exec("CREATE TABLE sensors (id INTEGER PRIMARY KEY, name TEXT, value REAL)");
        q.exec("INSERT INTO sensors (name, value) VALUES ('temperature', 23.5)");
        q.exec("INSERT INTO sensors (name, value) VALUES ('humidity', 61.2)");
        q.exec("INSERT INTO sensors (name, value) VALUES ('pressure', 1013.25)");

        q.exec("SELECT name, value FROM sensors");
        QStringList rows;
        while (q.next()) {
            QString row = QString("%1 = %2").arg(q.value(0).toString(),
                                                  q.value(1).toString());
            rows << row;
            printk("[sqlite] %s\n", qPrintable(row));
        }

        q.exec("SELECT COUNT(*) FROM sensors");
        q.next();
        int count = q.value(0).toInt();
        printk("[sqlite] %d rows in table\n", count);

        result = QString("OK: %1 rows\n%2").arg(count).arg(rows.join("\n"));
    }

    resultLabel->setText(result);

    return app.exec();
}
