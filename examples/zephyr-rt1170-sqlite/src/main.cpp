#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>

#include <zephyr/kernel.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/fs/fs.h>
#include <ff.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

extern "C" {
#include <sqlite3.h>
void sqlite_patch_vfs_for_zephyr(void);
}

static FATFS fat_fs;
static struct fs_mount_t mp = {
    .type = FS_FATFS,
    .fs_data = &fat_fs,
};

static bool mount_sd()
{
    if (disk_access_init("SD") != 0) {
        printk("[sqlite] disk_access_init failed\n");
        return false;
    }
    mp.mnt_point = "/SD:";
    if (fs_mount(&mp) != 0) {
        printk("[sqlite] mount failed\n");
        return false;
    }
    printk("[sqlite] SD card mounted at /SD:\n");
    return true;
}

static QString test_memory_db()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "mem");
    db.setDatabaseName(":memory:");

    if (!db.open())
        return QString("memory DB FAIL: %1").arg(db.lastError().text());

    QSqlQuery q(db);
    q.exec("CREATE TABLE t (id INTEGER PRIMARY KEY, v TEXT)");
    q.exec("INSERT INTO t (v) VALUES ('memory-test')");
    q.exec("SELECT v FROM t");
    q.next();
    QString v = q.value(0).toString();
    db.close();
    return QString("in-memory: OK (%1)").arg(v);
}

static QString test_file_db(const QString &path)
{
    bool existed = QFile::exists(path);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "file");
    db.setDatabaseName(path);

    if (!db.open())
        return QString("file DB FAIL: %1").arg(db.lastError().text());

    QSqlQuery q(db);
    q.exec("PRAGMA journal_mode=MEMORY");

    if (!existed) {
        q.exec("CREATE TABLE sensors (id INTEGER PRIMARY KEY, name TEXT, value REAL)");
        q.exec("INSERT INTO sensors (name, value) VALUES ('temperature', 23.5)");
        q.exec("INSERT INTO sensors (name, value) VALUES ('humidity', 61.2)");
        q.exec("INSERT INTO sensors (name, value) VALUES ('pressure', 1013.25)");
        printk("[sqlite] created new DB at %s\n", qPrintable(path));
    } else {
        printk("[sqlite] opened existing DB at %s\n", qPrintable(path));
    }

    q.exec("SELECT COUNT(*) FROM sensors");
    q.next();
    int count = q.value(0).toInt();

    q.exec("SELECT name, value FROM sensors");
    QStringList rows;
    while (q.next())
        rows << QString("%1 = %2").arg(q.value(0).toString(), q.value(1).toString());

    db.close();

    QString status = existed ? "existing" : "new";
    return QString("file DB (%1): %2 rows\n%3").arg(status).arg(count).arg(rows.join("\n"));
}

int main(int /*argc_zephyr*/, char * /*argv_zephyr*/[])
{
    printk("\n[sqlite] === boot ===\n");

    sqlite_patch_vfs_for_zephyr();

    bool sd_ok = mount_sd();

    static char arg0[] = "sqlite";
    static char *argv[] = { arg0, nullptr };
    int argc = 1;

    qputenv("QT_QPA_PLATFORM", "zephyr");
    QApplication app(argc, argv);

    QString result;
    result += test_memory_db() + "\n\n";

    if (sd_ok) {
        result += test_file_db("/SD:/sensors.db");
    } else {
        result += "SD card not available";
    }

    printk("[sqlite] result:\n%s\n", qPrintable(result));

    QWidget w;
    auto *layout = new QVBoxLayout(&w);

    auto *title = new QLabel("SQLite on Zephyr (SD card)");
    title->setStyleSheet("font-size: 28px; font-weight: bold;");
    layout->addWidget(title);

    auto *body = new QLabel(result);
    body->setStyleSheet("font-size: 22px;");
    body->setWordWrap(true);
    layout->addWidget(body);

    w.showFullScreen();
    return app.exec();
}
