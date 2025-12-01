#!/bin/sh

SOURCE=$HOME
BACKUP_EXCLUDE_LIST=backup-exclude.list


# print usage if no or invalid subcommand
print_usage()
{
   echo $0 \<subcommand\> # TODO ?? [directory] [directory] ...
#   echo [directory] defaults to user home directory if not given
   echo Files and directories listed in $BACKUP_EXCLUDE_LIST are excluded
   echo '          Subcommands:'
   echo 'du        Show disk usage of source, after files excluded'
   echo 'missex    Show files in the exclusion list which no longer exist'
#   echo 'filename  Show what filename would be used TODO'
   echo 'full      Do a full backup to a file in current dir'
#   echo 'from      Do a backup of files from specified modification date to present TODO'
}

# other subcommands below

# Show missing files/dirs in exclude list
# 'missex'
show_missing_excludes()
{
   echo TODO FIXME
}

# 'du'
show_disk_usage()
{
   du -amxX $SOURCE/backup-exclude.list $SOURCE | sort -n
}

# 'full'
full_backup()
{
   # TODO verify current dir isn't under homedir
   du -shxX $SOURCE/backup-exclude.list $SOURCE # show total

   DATE=`date -I`
   HOSTNAME=`uname -n`
   BASENAME=`basename $SOURCE`
   FILENAME=backup-$HOSTNAME-$BASENAME-$DATE.tbz
   echo Backing up to $FILENAME in 3 seconds using:
   echo bsdtar cjf $FILENAME -X $SOURCE/backup-exclude.list $SOURCE
   sleep 1
   echo Backing up to $FILENAME in 2 seconds...
   sleep 1
   echo Backing up to $FILENAME in 1 seconds...
   sleep 1
   # TODO run archive command and exit if it errors
   bsdtar cjf $FILENAME -X $SOURCE/backup-exclude.list $SOURCE || exit
   # verify num of files, should print archive errors too
   echo Backup done, verifying $FILENAME...
   COUNT=`bsdtar tvf $FILENAME | wc -l`
   echo $COUNT items backed up to $FILENAME
}

# 'from' backup files modified >= a provided date
from_backup()
{
   echo TODO FIXME
}

# main begins here

SUBCOMMMAND=$1
case $SUBCOMMMAND in
   missex)
      show_missing_excludes
      ;;
   du)
      show_disk_usage
      ;;
   full)
      full_backup
      ;;
   from)
      from_backup
      ;;
   *)
      print_usage
      ;;
esac
