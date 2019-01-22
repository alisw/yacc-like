curl -L https://github.com/westes/flex/releases/download/v2.6.4/flex-2.6.4.tar.gz | tar xzf -
curl -L https://ftp.gnu.org/gnu/bison/bison-3.2.4.tar.gz | tar xzf -
rsync -av --delete bison-*/ bison/
rsync -av --delete flex-*/ flex/
rm -rf flex-*
rm -fr bison-*
