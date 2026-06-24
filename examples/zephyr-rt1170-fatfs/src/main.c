#include <zephyr/kernel.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/fs/fs.h>
#include <ff.h>

static FATFS fat_fs;
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
};

static void lsdir(const char *path)
{
	struct fs_dir_t dir;
	struct fs_dirent entry;

	fs_dir_t_init(&dir);
	if (fs_opendir(&dir, path) != 0) {
		printk("[fatfs] opendir(%s) failed\n", path);
		return;
	}
	while (fs_readdir(&dir, &entry) == 0 && entry.name[0] != 0) {
		printk("[fatfs]   %s%s  %u bytes\n",
		       entry.name,
		       entry.type == FS_DIR_ENTRY_DIR ? "/" : "",
		       entry.size);
	}
	fs_closedir(&dir);
}

int main(void)
{
	printk("\n[fatfs] === SD card FAT test ===\n");

	static const char *disk = "SD";
	uint32_t sector_count = 0;
	uint32_t sector_size = 0;

	int rc_init;
	for (int retry = 0; retry < 5; retry++) {
		rc_init = disk_access_init(disk);
		if (rc_init == 0)
			break;
		printk("[fatfs] disk_access_init attempt %d failed: %d\n",
		       retry + 1, rc_init);
		printk("[fatfs] Is an SD card inserted? Retrying in 2s...\n");
		k_msleep(2000);
	}
	if (rc_init != 0) {
		printk("[fatfs] disk_access_init failed after retries."
		       " Insert an SD card and reset.\n");
		return 0;
	}
	disk_access_ioctl(disk, DISK_IOCTL_GET_SECTOR_COUNT, &sector_count);
	disk_access_ioctl(disk, DISK_IOCTL_GET_SECTOR_SIZE, &sector_size);
	printk("[fatfs] SD card: %u sectors x %u bytes = %u MB\n",
	       sector_count, sector_size,
	       (sector_count / 1024) * (sector_size / 1024));

	mp.mnt_point = "/SD:";
	int rc = fs_mount(&mp);
	if (rc != 0) {
		printk("[fatfs] mount failed: %d\n", rc);
		return 0;
	}
	printk("[fatfs] mounted at %s\n", mp.mnt_point);

	printk("[fatfs] root directory:\n");
	lsdir("/SD:");

	struct fs_file_t f;
	fs_file_t_init(&f);
	rc = fs_open(&f, "/SD:/zephyr_test.txt", FS_O_CREATE | FS_O_WRITE);
	if (rc == 0) {
		const char msg[] = "Hello from Zephyr on RT1170!\n";
		fs_write(&f, msg, sizeof(msg) - 1);
		fs_close(&f);
		printk("[fatfs] wrote zephyr_test.txt\n");
	} else {
		printk("[fatfs] open for write failed: %d\n", rc);
	}

	fs_file_t_init(&f);
	rc = fs_open(&f, "/SD:/zephyr_test.txt", FS_O_READ);
	if (rc == 0) {
		char buf[64];
		ssize_t n = fs_read(&f, buf, sizeof(buf) - 1);
		fs_close(&f);
		if (n > 0) {
			buf[n] = '\0';
			printk("[fatfs] read back: %s", buf);
		}
	} else {
		printk("[fatfs] open for read failed: %d\n", rc);
	}

	fs_unmount(&mp);
	printk("[fatfs] unmounted\n");
	printk("[fatfs] === done ===\n");
	return 0;
}
