# Preparing a PROJ Release

Preparing a PROJ release is a three-step process. Before the actual release is
published we want to issue one or more release candidates to ensure that the
code is in fact ready to be released. That is step one. Step two starts when a
release candidate has been selected for promotion to final release. Step three
updates version numbers in the master branch to avoid confusion between
released code and code still in development.

The three phases are described below.

## 1 Release candidate

At least one release candidate is necessary before the final release is
published. In case more than one release candidate is needed most steps below
can be skipped depending on what prompted the release candidate 1+n. Steps 1.6
and 1.7 are mandatory for all release candidates, otherwise it will in most
cases only be necessary to update the NEWS section (1.3).

Release candidates should be released about a week before the final release is
scheduled. When preparing major version releases it may be wise to issue the
release candidate earlier than that.

When a release candidate has not prompted any request for changes in 48 hours,
a motion for promotion to final release should be issued on the PROJ mailing
list. The PROJ PSC guidelines describes the rules for passing a motion.

### 1.1 Update ABI version numbers

In case of a un-planned maintenance release, make sure that version numbers are
properly updated: see 3.1

For each release version, increment PROJ_SOVERSION in CMakeLists.txt if the
ABI is not backwards compatible, i.e. any interfaces have been removed, or
changed since the last release. See also:
<https://community.kde.org/Policies/Binary_Compatibility_Issues_With_C%2B%2B>

*Commit the changes to master.*

### 1.2 Update man pages

Man pages are (mostly) automatically generated from the Sphinx docs. Follow
these steps:

- Temporarily update `version` and `data_version` in `docs/source/conf.py`
- run `scripts/update_man.sh`
- Revert version number in `docs/source/conf.py`

The `update_man.sh` script builds the man pages and commits the update man pages
to master.

### 1.3 Write release notes

Update `NEWS.md` with descriptions of the changes since the last release.
Hopefully issues and pull requests that go into the new release have been
properly tagged with the milestone for the current release.

End the section by thanking the authors that contributed to the release. Get a
sorted list of contributors in descending order of activity:

```sh
git shortlog -sn x.y.z..HEAD | tac | awk '{$1=""; print $0}'
```

where `x.y.z` is the tag of the previous release. You may need to edit the list
slightly since not all authors has properly configured their names in git.

*Commit the changes to master.*

### 1.4 Update `CITATION.cff` and `CITATION`

Update the "version" and "date-released" fields in `CITATION.cff`. The
"doi" does not need to be updated.

If needed, update the two instances of year in `CITATION`.

*Commit the changes to master.*

### 1.5 Update `PROJ_DATA.VERSION`

If the database references data from an updated PROJ-data package, update
`PROJ_DATA.VERSION` in `data/sql/metadata.sql`.

*Commit the changes to master.*

### 1.6 Create maintenance branch

*Skip this step if you are preparing a patch release or RC2 and later.*

Create the branch:

```sh
git branch x.y
```

where `x` is the major version number and `y` is the minor version number. Bug
fixes found in the release candidates should be pushed to master and
subsequently cherry-picked to the maintenance branch.

*Push branch upstream.*

### 1.7 Prepare and upload archives

Checkout the GitHub repo to an isolated local directory to package files only
tracked in git. Then, run CMake and build source distributions:

```sh
BRANCH_NAME=master
git clone --depth 1 --branch $BRANCH_NAME \
         https://github.com/OSGeo/PROJ.git PROJ-$BRANCH_NAME
cd PROJ-$BRANCH_NAME
mkdir _build && cd _build
cmake -D BUILD_TESTING=OFF ..
cmake --build . --target dist
```

Rename the archives and generate MD5 sum files:

```sh
mv proj-x.y.z.tar.gz proj-x.y.zRCn.tar.gz
mv proj-x.y.z.zip proj-x.y.zRCn.zip
md5 proj-x.y.zRCn.tar.gz > proj-x.y.zRCn.tar.gz.md5
md5 proj-x.y.zRCn.zip > proj-x.y.zRCn.zip.md5
```

Upload to the OSGeo download server:

```sh
scp proj-x.y.zRCn.* warmerdam@upload.osgeo.org:/osgeo/download/proj
```

Adjust version numbers and usernames accordingly.

### 1.8 Announce the release candidate

Announce the release candidate on the PROJ mailing list.

### 1.9 Promotion to final release

When you are confident that the latest release candidate is ready for promotion
to final release a motion for promotion should be issued on the PROJ mailing
list. Allow for a minimum of 48 hours for voting time.

## 2 Final release

Barring a successful promotion process we can proceed with the final release.

### 2.1 Prepare and upload archives

See step 1.6 above. Do not rename the archives with RC postfixes.


### 2.2 Update the STABLE_BRANCH repository variable

Go to https://github.com/OSGeo/PROJ/settings/variables/actions/STABLE_BRANCH
and update the value to be x.y

### 2.3 Update website

- Add the release notes from `NEWS.md` to `docs/source/news.rst`

  These vim substitutions are helpful when converting NEWS.md to rst:

```sh
:s!(\#\(\d\{4}\))!(`\#\1 <https://github.com/OSGeo/PROJ/issues/\1>`_)!
```

- Update download links in `docs/source/download.rst`

*Commit the changes to master and cherry-pick into maintenance branch.*

When pushed upstream the website will be update to the current version.

### 2.4 Tag the release

Tag the release with:

```sh
git tag -a -m "Create tag x.y.z" x.y.z
git push --tags
```

### 2.5 Check the GitHub Zeonodo integration

Go to https://zenodo.org/account/settings/github/ and check that the OSGeo/PROJ
repository is ON so that the creation of the GitHub release triggers the
creation of a corresponding Zenodo record.

### 2.6 Prepare the release on GitHub

When the new tag is pushed upstream GitHub picks up that a new release has been
issued. Update the new release on <https://github.com/OSGeo/PROJ/releases> with
the release notes from `NEWS.md` and add the prepared source distribution archives
to the release (otherwise GitHub will just package the entire repository - we
don't want that).

### 2.7 Announce the new release

The release should be announced on PROJ and MetaCRS mailing lists. Additionally
the release announcement should be sent to <news_item@osgeo.org> which will add
the announcement to osgeo.org and other OSGeo communication channels.

Make some noise on relevant social media.

## 3 Post final release

### 3.1 Update version numbers

PROJ uses [semantic versioning](http://semver.org/). Depending on the changes
in the release will be either a major, minor or patch release. Update the
version numbers in the files.

- src/proj.h:           Update PROJ_VERSION_MAJOR, PROJ_VERSION_MINOR and
                        PROJ_VERSION_PATCH.
- src/release.cpp:      Update date to the scheduled release date.
                        Possibly update the wisdom of the day sentence.
- CMakeLists.txt:       Update proj_version()
- docs/source/conf.py:  Update version number

*Commit changes. Either to master or maintenance branch depending on the nature
of the release.*

### 3.2 Documentation

Edit the Sphinx configuration docs/source/conf.py with these changes:

- Set "version" to the major project version, e.g. "9.2"
- Set "release" to the full project version, e.g. "9.2.1"
- Set "data_version" to the PROJ-data version
- Set "today_date = date(Y, M, D)" using the release date
- set "github_version" to the maintenance branch label, e.g. "9.2"

HTML and PDF documentation is built using ReadTheDocs, which supports multiple
versions. Versions are based on branch labels, e.g. "9.2". To modify settings,
registered users must first log in. To add a version, go to
<https://readthedocs.org/projects/osgeo-proj/versions/> and click "Activate"
next to the maintenance branch label.
