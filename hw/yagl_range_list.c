#include "yagl_range_list.h"

struct yagl_range
{
    int start;
    int size;
};

void yagl_range_list_init(struct yagl_range_list *range_list)
{
    yagl_vector_init(&range_list->ranges, sizeof(struct yagl_range), 0);
}

void yagl_range_list_cleanup(struct yagl_range_list *range_list)
{
    yagl_vector_cleanup(&range_list->ranges);
}

void yagl_range_list_add(struct yagl_range_list *range_list,
                         int start,
                         int size)
{
    int actual_start = start, actual_end = start + size;
    int remove_first = -1, remove_last = -1;
    int i;
    struct yagl_range *ranges = yagl_vector_data(&range_list->ranges);
    int num_ranges = yagl_vector_size(&range_list->ranges);

    for (i = 0; i < num_ranges; ++i) {
        if ((remove_first == -1) &&
            ((ranges[i].start + ranges[i].size) >= start)) {
            if (ranges[i].start < actual_start) {
                actual_start = ranges[i].start;
            }
            remove_first = i;
            remove_last = i;
        }
        if (remove_first != -1) {
            if ((ranges[i].start <= actual_end)) {
                if ((ranges[i].start + ranges[i].size) > actual_end) {
                    actual_end = (ranges[i].start + ranges[i].size);
                }
                remove_last = i + 1;
            } else {
                break;
            }
        }
    }

    if (remove_first == -1) {
        remove_first = 0;
        remove_last = 0;
    }

    if (remove_first == remove_last) {
        yagl_vector_resize(&range_list->ranges, num_ranges + 1);
        ranges = yagl_vector_data(&range_list->ranges);
        memmove(ranges + remove_first + 1,
                ranges + remove_first,
                (num_ranges - remove_first) * sizeof(*ranges));
    } else {
        memmove(ranges + remove_first + 1,
                ranges + remove_last,
                (num_ranges - remove_last) * sizeof(*ranges));
        yagl_vector_resize(&range_list->ranges,
                           num_ranges + 1 - (remove_last - remove_first));
        ranges = yagl_vector_data(&range_list->ranges);
    }

    ranges[remove_first].start = actual_start;
    ranges[remove_first].size = (actual_end - actual_start);
}

int yagl_range_list_size(struct yagl_range_list *range_list)
{
    return yagl_vector_size(&range_list->ranges);
}

void yagl_range_list_get(struct yagl_range_list *range_list,
                         int i,
                         int *start,
                         int *size)
{
    struct yagl_range *ranges =
        yagl_vector_data(&range_list->ranges);

    *start = ranges[i].start;
    *size = ranges[i].size;
}

void yagl_range_list_clear(struct yagl_range_list *range_list)
{
    yagl_vector_resize(&range_list->ranges, 0);
}
