#include "network.hh"
#include "menu_shared.hh"
#include "gui.hh"
#include "pkgman.hh"
#include <algorithm>
#include <tms/cpp.hh>

/* Publish level variables */
uint32_t      _publish_lvl_community_id;
uint32_t      _publish_lvl_id;
volatile bool _publish_lvl_uploading = false;
bool          _publish_lvl_uploading_error = false;

/* Download pkg variables */
uint32_t      _play_pkg_id;
uint32_t      _play_pkg_type;
uint32_t      _play_pkg_downloading = false;
uint32_t      _play_pkg_downloading_error = false;

/* Download level variables */
uint32_t      _play_id;
char          _community_host[512] = {0}; /* Temporary input host from principia:// url, not to be confused with P.community_host */
uint32_t      _play_type;
bool          _play_lock;
volatile bool _play_downloading = false;
volatile bool _play_download_for_pkg = false;
volatile int  _play_downloading_error = 0;
struct header_data _play_header_data = {0};

/* Submit score variables */
bool _submit_score_done = false;

bool load_featured_cache(const char *path, char *buf, size_t *buf_size) {
	FILE *fh = fopen(path, "rb");
	if (fh) {
		fseek(fh, 0, SEEK_END);
		*buf_size = ftell(fh);
		fseek(fh, 0, SEEK_SET);

		fread(buf, 1, *buf_size, fh);

		fclose(fh);
	} else {
		tms_infof("Error opening cache file!");
		return false;
	}

	return true;
}

void save_featured_cache(const char *path, char *buf, size_t buf_size) {
	FILE *fh = fopen(path, "wb");

	if (fh) {
		fwrite(buf, 1, buf_size, fh);

		fclose(fh);
	}
}

bool parse_featured_levels(char *buf, size_t buf_size) {
    lvlbuf lb;
    lb.reset();
    lb.size = 0;
    lb.ensure(buf_size);

    lb.buf = (uint8_t*)buf;
    lb.size = buf_size;

    tms_debugf("featured data size: %d", (int)buf_size);

    uint32_t count = lb.r_uint32();

    uint32_t n = std::min(count, 4u);
    for (uint32_t i = 0; i < n; ++i) {
        menu_shared::fl[i].id = lb.r_uint32();

        uint32_t name_len = lb.r_uint32();
        char *name = (char*)calloc(name_len+1, 1);
        lb.r_buf(name, name_len);
        strcpy(menu_shared::fl[i].name, name);
        free(name);

        uint32_t creator_len = lb.r_uint32();
        char *creator = (char*)calloc(creator_len+1, 1);
        lb.r_buf(creator, creator_len);
        strcpy(menu_shared::fl[i].creator, creator);
        free(creator);

        uint32_t thumb_len = lb.r_uint32();

        if (thumb_len == 0) {
            tms_errorf("Featured level received with no thumbnail!");
            continue;
        }
        char *thumb = (char*)malloc(thumb_len);
        lb.r_buf(thumb, thumb_len);

        tms::texture *tex = new tms::texture();
        tex->load_mem2(thumb, thumb_len, 0, "jpg");
        tex->flip_y();
        tex->add_alpha(1.f);

        menu_shared::fl[i].sprite = tms_atlas_add_bitmap(
                gui_spritesheet::atlas,
                tex->width,
                tex->height,
                tex->num_channels,
                tex->data
                );

        free(thumb);
        tex->free_buffer();

        delete tex;
    }

    uint32_t contest_active = lb.r_uint32(); // unused 32-bit int

    uint32_t num_getting_started_links = lb.r_uint32();

    tms_infof("Num getting started links: %u", num_getting_started_links);

    menu_shared::gs_entries.clear();

    for (uint32_t x=0; x<num_getting_started_links; ++x) {
        uint32_t title_len, link_len;
        char    *title,    *link;

        title_len = lb.r_uint32();
        title = (char*)calloc(title_len+1, 1);
        lb.r_buf(title, title_len);

        link_len = lb.r_uint32();
        link = (char*)calloc(link_len+1, 1);
        lb.r_buf(link, link_len);

        menu_shared::gs_entries.push_back(gs_entry(strdup(title), strdup(link)));

        free(title);
        free(link);
    }

    if (num_getting_started_links) {
        menu_shared::gs_state = FL_WAITING;
    }

	return true;
}
