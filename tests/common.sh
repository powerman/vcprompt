# where to find all the test files
testdir=`pwd`

# other globals set by functions here
vcprompt=""
tmpdir=""

die()
{
    msg=$1
    echo "error: $msg" >&2
    exit 1
}

# Prepend $TOOLPATH to $PATH (if $TOOLPATH set), so caller can
# influence where external tools (e.g. svn, hg) are found. Must be
# called before check_available if it's to be of any use.
set_path()
{
    if [ -s "$TOOLPATH" ]; then
        PATH=$TOOLPATH:$PATH
    fi
}

# Check if some external command is available by running it
# and ensuring that it prints an expected string.  If not,
# exit with optional message.
check_available()
{
    cmd=$1
    expect=$2
    msg=$3

    if ! $cmd 2>/dev/null | grep -q "$expect"; then
        [ "$msg" ] && echo $msg
        exit 0
    fi
}

find_vcprompt()
{
    vcprompt=$testdir/../vcprompt
    [ -x $vcprompt ] ||
        die "vcprompt executable not found (expected $vcprompt)"
}

# Check if some feature was built in to the current vcprompt binary;
# exit with a message if not.
check_feature()
{
    feature=$1
    msg=$2
    if ! $vcprompt -F | fgrep -q -x -e "$feature"; then
        echo $msg
        exit 0
    fi
}

setup()
{
    tmpdir=`mktemp -d /tmp/vcprompt.XXXXXX`
    [ -n "$tmpdir" -a -d "$tmpdir" ] ||
        die "unable to create temp dir '$tmpdir'"
    trap cleanup 0 1 2 15
}

cleanup()
{
    echo "cleaning up $tmpdir"
    chmod -R u+rwx $tmpdir
    rm -rf $tmpdir
}

assert_vcprompt()
{
    message=$1
    expect=$2
    format=$3
    if [ -z "$format" ]; then
        format="%b"
    fi

    prefix=""
    if [ "$VCPVALGRIND" ]; then
        prefix="valgrind --leak-check=full --error-exitcode=99 -q "
    fi

    if [ "$format" != '-' ]; then
        cmd='VCPROMPT_FORMAT="$VCPROMPT_FORMAT" $prefix$vcprompt -f "$format"'
    else
        cmd='VCPROMPT_FORMAT="$VCPROMPT_FORMAT" $prefix$vcprompt'
    fi
    actual=`eval $cmd`
    status=$?

    if [ $status -ne 0 ]; then
        echo "fail: $message: child process terminated with exit status $status; command was:" >&2
        eval echo $cmd >&2
        failed="y"
        return $status
    elif [ "$expect" != "$actual" ]; then
        echo "fail: $message: expected \"$expect\", but got \"$actual\"" >&2
        failed="y"
        return 1
    else
        echo "pass: $message"
    fi
}

report()
{
    if [ "$failed" ]; then
	echo "$0: some tests failed"
	exit 1
    else
	echo "$0: all tests passed"
	exit 0
    fi
}
