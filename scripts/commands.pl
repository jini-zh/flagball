#!/usr/bin/perl

use Cwd;

@int_data = qw(default min max description);
@str_data = qw(default description);

sub strip_quotes {
 for ($_[0]) {
  s/^"//;
  s/"$//;
  return $_;
 };
};

sub cpp {
 open my $result, 
      '-|', 
      'cpp '
      . join ' ', map { "'$_'" } (
         '-I', '.',
         '-iquote', '/usr/include/SDL',
         '-I', '/usr/include/freetype2',
         '-I', '/usr/include/wavpack',
         @_
        )
  or die "cpp $_[-1]: $!\n";
 return $result;
};

sub read_variables {
 my %int;
 my %str;
 foreach my $file ('engine/shared/config_variables.h', 'game/variables.h') {
  my $cpp = cpp 
    '-DMACRO_CONFIG_INT(slot, command, default, min, max, flags, description)=flags@command@int@default@min@max@description',
    '-DMACRO_CONFIG_STR(slot, command, len, default, flags, description)=flags@command@str@default@description',
    '-DSERVERINFO_LEVEL_MIN=0',
    $file;
  while (<$cpp>) {
   next if /^#/ || /^\s*$/;
   chomp;
   my @data = split /@/;
   next unless shift(@data) =~ /CFGFLAG_SERVER/;
   my $command = shift @data;
   strip_quotes $data[-1];
   if (shift @data eq 'int') {
    @{$int{$command}}{@int_data} = @data;
   } else {
    strip_quotes $data[0];
    @{$str{$command}}{@str_data} = @data;
   };
  };
  close $cpp or exit $? >> 8;
 };
 return (\%int, \%str);
};

sub do_file_tree {
 my ($tree, $function) = @_;
 if (-d $tree) {
  opendir my $d, $tree or die "Cannot open dir $tree: $!\n";
  while ($_ = readdir $d) {
   next if $_ eq '.' || $_ eq '..';
   do_file_tree($tree . '/' . $_, $function);
  };
  closedir $d;
 } else {
  $function->($tree);
 };
};

sub read_commands {
 my %commands;
 do_file_tree '.', sub {
  my ($file) = @_;
  return if $file !~ /\.cpp$/ || $file =~ m{/test/};
  my $cpp = cpp 
    '-DRegister(command, params, flags, func, user, help)=!!REGISTER@flags@command@params@help@',
    $file;
  while (<$cpp>) {
   next unless /!!REGISTER/;
   @_ = split /@/;
   shift;
   next unless shift() =~ /CFGFLAG_SERVER/;
   my ($command, $params, $help) = map { strip_quotes $_ } @_;
   $commands{$command} = { args => $params, description => $help };
  };
  close $cpp or exit $? >> 8;
 };
 return \%commands;
};

sub max {
 my $result = shift;
 foreach (@_) {
  $result = $_ if $_ > $result;
 };
 return $result;
};

sub print_table {
 my ($header, $data) = @_;
 my @keys = sort { $a cmp $b } keys %$data;

 my $h0 = $header->[0];
 my $s0 = length $h0;
 my @h = @$header[1 .. $#{$header}];
 my @sizes = map { length } @h;
 foreach (@keys) {
  for (my $i = 0; $i < @h; ++$i) {
   $sizes[$i] = max $sizes[$i], length $data->{$_}{$h[$i]};
  };
  $s0 = max $s0, length;
 };

 my $f;
 $f .= ' | %-' . $_ . 's' foreach @sizes;
 my $h = sprintf '%-' . ($s0 + 2) . 's' . $f, 
                 map { (my $x = $_) =~ s/./uc($&)/e; $x } @$header;
 print $h, "\n";
 $h =~ s/[^|]/-/g;
 print $h, "\n";
 $f .= "\n";
 foreach (@keys) {
  print '`', $_, '`', ' ' x ($s0 - length);
  printf $f, @{$data->{$_}}{@h};
 };
};

sub make_footnotes {
 my $index = keys %footnote || 1;
 while ($_ = shift) {
  $footnote{$_}{ref}  = '[(' . $index++ . ")][$_]";
  $footnote{$_}{text} = shift;
 };
};

sub footnote {
 my ($variable, $note) = @_;
 exists $footnote{$note} or die "No such footnote: $note\n";
 $variable->{description} .= ' ' . $footnote{$note}->{ref};
};

sub print_footnotes {
 my $index = sub { $_[0] =~ /\[\((\d+)\)\]/; return $1 };
 my @refs = sort {
  $index->($footnote{$a}{ref}) <=> $index->($footnote{$b}{ref}) 
 } keys %footnote;
 my $w = 0;
 $w = max $w, length foreach @refs;
 print "[$_]: ", ' ' x ($w - length), '#', $_, "\n" foreach @refs;
 print "\n";
 print '<a name="', $_, '">(', $index->($footnote{$_}->{ref}), ')</a> ', 
       ' ' x ($w - length), $footnote{$_}{text}, "\n\n"
  foreach @refs;
};

$root = $0;
$root =~ s#scripts/[^/]*##;
$root .= 'src';
chdir $root or die "Cannot chdir to $root: $!\n";

($int, $str) = read_variables;
$commands = read_commands '.';

foreach (keys %$commands) {
 delete $commands->{$_} if exists($int->{$_}) || exists($str->{$_});
};

make_footnotes
 boolean   => 'This is a boolean value. 0 means disabled, 1&mdash;enabled.',
 zero      => 'Zero value disables this option.';

foreach (values %$int) {
 if ($_->{min} == 0 && $_->{max} == 1) {
  footnote $_, 'boolean';
 } elsif ($_->{max} == 0) {
  $_->{max} = '&mdash;';
 }
};

footnote $int->{$_}, 'zero'
  foreach qw(
   sv_fb_owngoal_kick
   sv_inactivekick_time
   sv_scorelimit
   sv_teambalance_time
   sv_timelimit
  );

print <<END;
title:flagball commands

# Variables

## Integer variables

END
print_table [ 'variable', @int_data ], $int;
print <<END;

## String variables

END
print_table [ 'variable', @str_data ], $str;
print "\n";
print_footnotes;
print <<END;

## Commands

END
print_table [ 'command', 'args', 'description' ], $commands;
