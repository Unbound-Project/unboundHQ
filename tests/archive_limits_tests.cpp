#include "archive_limits.hpp"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <limits>

int main() {
    using unbound::ArchiveSizeLimit;
    using unbound::archiveSizeLimit;
    constexpr std::uint64_t GiB = 1024ULL * 1024 * 1024;
    static_assert(unbound::config::MaxArchiveBytes == 2 * GiB);
    static_assert(unbound::config::MaxArchiveEntryBytes == 2 * GiB);
    static_assert(unbound::config::MaxArchiveExpandedBytes == 5 * GiB);

    assert(archiveSizeLimit(0, 0) == ArchiveSizeLimit::None);
    assert(archiveSizeLimit(0, 2 * GiB) == ArchiveSizeLimit::None);
    assert(archiveSizeLimit(0, 2 * GiB + 1) == ArchiveSizeLimit::Entry);
    assert(archiveSizeLimit(3 * GiB, 2 * GiB) == ArchiveSizeLimit::None);
    assert(archiveSizeLimit(3 * GiB + 1, 2 * GiB) == ArchiveSizeLimit::Expanded);
    assert(archiveSizeLimit(5 * GiB, 0) == ArchiveSizeLimit::None);
    assert(archiveSizeLimit(5 * GiB, 1) == ArchiveSizeLimit::Expanded);
    assert(archiveSizeLimit(5 * GiB + 1, 0) == ArchiveSizeLimit::Expanded);
    assert(archiveSizeLimit(std::numeric_limits<std::uint64_t>::max(), 1) ==
           ArchiveSizeLimit::Expanded);
    assert(archiveSizeLimit(0, std::numeric_limits<std::uint64_t>::max()) ==
           ArchiveSizeLimit::Entry);

    // The published 0.2.0 package expands 8,064,802 bytes beyond the old total
    // ceiling. Exercise that exact aggregate without allocating a giant file.
    std::uint64_t expanded = 0;
    for (const std::uint64_t entry : {GiB / 2, GiB / 2, GiB / 2, GiB / 2,
                                      std::uint64_t{8064802}}) {
        assert(archiveSizeLimit(expanded, entry) == ArchiveSizeLimit::None);
        expanded += entry;
    }
    assert(expanded == 2155548450ULL);
    assert(expanded > unbound::config::MaxArchiveBytes);

    // Five GiB is accepted across ordinary entries, but never a six-GiB total.
    expanded = 0;
    for (const std::uint64_t entry : {2 * GiB, 2 * GiB, GiB}) {
        assert(archiveSizeLimit(expanded, entry) == ArchiveSizeLimit::None);
        expanded += entry;
    }
    assert(expanded == 5 * GiB);
    assert(archiveSizeLimit(expanded, GiB) == ArchiveSizeLimit::Expanded);
    std::puts("Archive limit tests passed");
}
