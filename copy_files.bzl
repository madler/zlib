"""Contains the copy_files macro for moving files."""
load("@bazel_skylib//rules:copy_file.bzl", "copy_file")

def copy_files(name, srcs, subdirectory):
    """Copy files to the subdirectory.

    Every file is copied in a separate target using the target label
    "$name_$src.basename". A filegroup target "$name" contains all the
    copied files.

    Args:
        name: Name of filegroup containing all files.
        srcs: The relative filepaths of the files to copy.
        subdirectory: The subdirectory path to copy the files into.
    """
    outs = []
    for src in srcs:
        basename = src.split('/')[-1]
        src_label = name + "_" + basename
        copy_file(
            name = src_label,
            src = src,
            out = subdirectory + '/' + src,
        )
        outs.append(src_label)
    native.filegroup(
        name = name,
        srcs = outs,
    )


