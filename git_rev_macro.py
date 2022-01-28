import subprocess
import datetime

date = datetime.datetime.now()

date = date.strftime("%Y-%m-%d %H:%M:%S %Z")

try:
    version = (
        subprocess.check_output(["git", "describe", "--tags", "--abbrev=0", "--dirty=\'m\'"])
        .strip()
        .decode("utf-8")
        )
except subprocess.CalledProcessError:
    version = 'vdev'

try:
    git_version = (
        subprocess.check_output(["git", "describe", "--abbrev=8", "--always", "--tags", "--dirty=\' (modified)\'"])
        .strip()
        .decode("utf-8")
        )
except subprocess.CalledProcessError:
    git_version = '?'

print("-DVERSION='\"{}\"' -DBUILD_DATE='\"{}\"' -DGIT_VERSION='\"{}\"'".format(version, date, git_version))
