# CS377 group project - Chat System
#### Members: DongWon Park, Shipeng Yu, 

### How to use Git for the project
### Set up git environment
<pre>
1. Create a folder (don't use cloud drive because it is unstable to work with github)
+2. Download this repository
        <b>sh> git clone https://github.com/wesnjazz/CS377_chatsystem.git</b>
3. go to CS377_chatsystem folder.
4. check your git status
        <b>sh> git status</b>
5. check branches that already made. You will see there are individual branches for each member.
   You need to work on only your branch.
        <b>sh> git branch -a</b>
6. checkout to your branch
        <b>sh> git checkout shipeng</b>   or
        <b>sh> git checkout ruifeng</b>
7. Now you are done with setup.
</pre>

### Branches
<pre>
There are five branches.
[develop] : The branch we will merge our work in one place as we develop our project. 
            This branch is made to protect the [master] branch.
[shipeng, ruifeng, dpark] : These are individual branch that we work on separately. Do your work only on your branch.
[master] : We will finalize our project to this branch. This branch will be always stable without errors.
</pre>

### When you do your tasks
<pre>
1. Always work on your branch
    <b>sh> git checkout [mybranch]</b>
2. Do your job!
</pre>

### After completing every each small task
<pre>
Part 1: SAVE YOUR JOB
1. <b>git add .</b>
   this will add all your changed files to be tracked.
2. <b>git commit -m "small description what you did"</b> 
   ex) git commit -m "added a helpfer function for socket binding, fixed a segfault bug"
   this will save/commit your state in your LOCAL git
3. <b>git push</b>
   this will sync your local and github

PARt 2: PULL REQUEST (Merge your work into the main flow)
4. <b>Go to [github page](https://github.com/wesnjazz/CS377_chatsystem)</b>
5. <b>Select your branch</b>
6. <b>New pull request</b>
7. <b>Base: Develop <- Compare:your branch</b>
8. <b>Create Pull Request</b>
</pre>

### After Pull Request
<pre>
Merge [develop] branch into your branch
1. <b>git checkout develop</b>
2. <b>git pull</b> (Update your local develop branch up-to-date with github)
3. <b>git checkout [mybranch]</b>
4. <b>git pull</b> (Update your branch)
5. <b>git merge develop</b> (Merge develop branch into your branch)
6. <b>git push</b>
   push again to make your change(merging) being published into github.
   Now you are up-to-date and ready to develop next level!
</pre>

### 

<br><br>
Last update: 2018.12.01.
