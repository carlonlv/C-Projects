The goal of this project is to practice system calls and dynamic memory allocation.  
This program explores the /proc virtual file system to generate (and display) a tree of the processes (the executing programs) on the system.  
The end result will be output that describes the children of a given process, much like the pstree command does as show in this example: 
$ pstree -p 1123  
sshd(1123)---sshd(25914)---sshd(25987)---bash(25988)---pstree(1243)  
           |-sshd(27395)---sshd(27468)---bash(27469)  
           |-sshd(29462)---sshd(29498)---bash(29531)  
           |-sshd(29499)---sshd(29529)---sftp-server(29530)  
           |-sshd(29874)---sshd(29912)---bash(29913)  
           |-sshd(30932)---sshd(30962)---sftp-server(30963)  
           |                           |-sftp-server(30964)  
           |                           |-sftp-server(30965)  
           |-sshd(31511)---sshd(31547)---bash(31548)  
