This is a basic set of instructions for STAR users who are not familiar with
Git. In the following we assume that the `git` version is at least 2.17.0 just
like the one installed on RACF for STAR users. We also use the `star-cvs` name
for the Git repository and the remote location at
https://github.com/star-bnl/star-cvs.git  which is likely to change.


## How to get the STAR code from the Git repository

The easiest way to get a local copy of the entire codebase with the history of
changes is to do:

    $ git clone https://github.com/star-bnl/star-cvs.git

Then `cd` into the newly created directory, look around, and browse the history
with one of the popular utilities, e.g. `gitk` or `tig` (usualy available along
with `git`):

    $ cd star-cvs
    $ ls -a
    .  ..  asps  .git  kumacs  mgr  OnlTools  pams  StarDb  StarVMC  StDb  StRoot
    $ git status
    On branch master
    Your branch is up to date with 'origin/master'.
    
    nothing to commit, working tree clean
    $ tig


## How to checkout only one or a few packages/subdirectories

This is called a sparse checkout. In this case you start by cloning the bare
repository

    $ git clone --no-checkout https://github.com/star-bnl/star-cvs.git
    $ ls -a star-cvs/
    .  ..  .git

Note that the above command will still create a local copy of the entire
history in `.git` so you can switch between different versions later (There is
also a way to limit the amount of cloned history). Let `git` know that you want
to work with a limited number of modules

    $ cd star-cvs/
    $ git config core.sparseCheckout true

Now create and modify the `.git/info/sparse-checkout` file to include a list of
packages/subdirectories you want to work with locally. The contents of the file
may look like this

    $ cat .git/info/sparse-checkout
    StRoot/Sti

or like this

    $ cat .git/info/sparse-checkout
    StRoot/StTofCalibMaker
    StRoot/StTofHitMaker
    StRoot/StTofMaker

wild cards are also possible to use

    $ cat .git/info/sparse-checkout
    StRoot/StPico*

Finally, ask `git` to checkout the selected subdirectories

    $ git checkout SL20a
    $ ls -a StRoot/
    .  ..  StPicoDstMaker  StPicoEvent

Assuming the default STAR environment on a RACF interactive node the code can be
compiled as usual with the `cons` command.
