set -eu
set -o pipefail

LAST_VERSION="1.62.0"
NEW_VERSION="1.63.0"

: ' 

manual intervention:

  - change/upgrade icu version used by boost_regex
  - new libraries available to build?

'

CLEAN="${CLEAN:-false}"


if [[ ${CLEAN} ]]; then
    rm -rf scripts/boost/${NEW_VERSION}
fi

mkdir scripts/boost/${NEW_VERSION}
cp -r scripts/boost/${LAST_VERSION}/. scripts/boost/${NEW_VERSION}/
perl -i -p -e "s/MASON_VERSION=${LAST_VERSION}/MASON_VERSION=${NEW_VERSION}/g;" scripts/boost/${NEW_VERSION}/base.sh 
export BOOST_VERSION=${NEW_VERSION//./_}
export CACHE_PATH="mason_packages/.cache"
mkdir -p "${CACHE_PATH}"
if [[ ! -f ${CACHE_PATH}/boost-${NEW_VERSION} ]]; then
    curl --retry 3 -f -S -L http://downloads.sourceforge.net/project/boost/boost/${NEW_VERSION}/boost_${BOOST_VERSION}.tar.bz2 -o ${CACHE_PATH}/boost-${NEW_VERSION}
fi

NEW_SHASUM=$(git hash-object ${CACHE_PATH}/boost-${NEW_VERSION})

perl -i -p -e "s/BOOST_SHASUM=(.*)/BOOST_SHASUM=${NEW_SHASUM}/g;" scripts/boost/${NEW_VERSION}/base.sh 

for lib in $(find scripts/ -maxdepth 1 -type dir -name 'boost_lib*' -print); do
    if [[ -d $lib/${LAST_VERSION} ]]; then
        if [[ ${CLEAN} ]]; then
            rm -rf $lib/${NEW_VERSION}
        fi
        mkdir $lib/${NEW_VERSION}
        cp -r $lib/${LAST_VERSION}/. $lib/${NEW_VERSION}/
    else
        echo "skipping creating package for $lib"
    fi
done

./mason trigger boost ${NEW_VERSION}
# TODO: this is rate limited so it needs to be run over many hours to avoid travis blocking
for lib in $(find scripts/ -maxdepth 1 -type dir -name 'boost_lib*' -print); do
    echo ./mason trigger $(basename $lib) ${NEW_VERSION}
done

