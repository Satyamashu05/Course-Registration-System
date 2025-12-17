#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<stdlib.h>
#define PORT 8080

typedef struct{
    int id;
    char name[30];
    char password[30];
    int active; // 1 = active, 0 = inactive
}student;

typedef struct{
    int id;
    char name[30];
    char password[30];
}faculty;

typedef struct{
    int id;
    char name[30];
    int numEnroll;
}course;

void adminClient(int sock){
    printf(".......................Welcome to Admin menu.....................\n");
    
    while(1){
        char request[1024];
        char response[1024];
        int choice;
        printf("Do you want to:\n1. Add Student\n2. Add Faculty\n3. Activate/Deactivate Student\n4. Update Student/Faculty details\n5. Exit\n");
        printf("Enter your choice: ");
        scanf("%d",&choice);
        if(choice == 5)break;
        switch(choice){
            case 1:
            int rollNum;
            char studentName[30];
            char password[30];
            printf("Enter roll number of student: ");
            scanf("%d",&rollNum);
            printf("Enter name of student: ");
            scanf("%s",studentName);
            printf("Enter password: ");
            scanf("%s",password);
            sprintf(request,"ADD_STUDENT|%d|%s|%s",rollNum,studentName,password);
            break;

            case 2:
            int facultyId;
            char facultyName[30];
            password[30];
            printf("Enter the id number of faculty: ");
            scanf("%d",&facultyId);
            printf("Enter name of faculty: ");
            scanf("%s",facultyName);
            printf("Enter the password:");
            scanf("%s",password);
            sprintf(request,"ADD_FACULTY|%d|%s|%s",facultyId,facultyName,password);
            break;

            case 3:
            int id,a; 
            printf("Enter Student id: ");
            scanf("%d",&id);
            printf("Enter 0 if you want to activate and 1 for deactivate: ");            
            scanf("%d",&a);          
            if(!a)sprintf(request,"ACTIVATE_STUDENT|%d",id);
            else sprintf(request,"DEACTIVATE_STUDENT|%d",id);            
            break;


            case 4:
            printf("Enter 1 if you want to update student details and 2 for faculty: ");
            a;
            scanf("%d",&a);
            if(a==1){
                int rollNum;char name[30];
                printf("Enter roll number and name of student: ");
                scanf("%d %s",&rollNum,name);
                sprintf(request,"UPDATE_STUDENT|%d|%s",rollNum,name);
            }else if(a==2){
                int facultyId;char name[30];
                printf("Enter facultyID and name of faculty: ");
                scanf("%d %s",&facultyId,name);
                sprintf(request,"UPDATE_FACULTY|%d|%s",facultyId,name);
            }else printf("Invalid key\n");
            break;

            default:
            printf("Invalid choice\n");
        }
        send(sock, request, sizeof(request),0);
        int bytes_received = read(sock, response, sizeof(response) - 1);  // Leave space for null terminator
        if (bytes_received > 0) {
            response[bytes_received] = '\0';  // Null-terminate the received data
            printf("Server response: %s\n", response);
        } else if (bytes_received == 0) {
            printf("Server closed the connection\n");
        } else {
            perror("Error reading from socket");
        }
    }    
}

void facultyClient(int sock,int facultyId){
    printf(".......................Welcome to Faculty menu.....................\n");
    
  
    while(1){
        char request[1024];
        char response[1024];
        int choice;
        printf("Do you want to:\n1. Add new Course\n2. Remove offered Course\n3. View enrollments in Courses\n4. Password Change\n5. Exit\n");
        printf("Enter your choice:\n");
        scanf("%d",&choice);
        if(choice==5)break;

        switch(choice){
            case 1:
            int courseId;
            char name[30];
            int credits;
            printf("Enter courseId: ");
            scanf("%d",&courseId);
            printf("Enter course name: ");
            scanf("%s",name);
            printf("Enter number of credits: ");
            scanf("%d",&credits);
            sprintf(request,"ADD_COURSE|%d|%d|%s|%d",facultyId,courseId,name,credits);
            break;

            case 2:
            courseId;           
            printf("Enter courseId: ");
            scanf("%d",&courseId);
            sprintf(request,"REMOVE_COURSE|%d|%d",facultyId,courseId);
            break;

            case 3:
            sprintf(request,"VIEW_ENROLLMENTS|%d",facultyId);
            break;

            case 4:
            sprintf(request, "CHANGE_PASS|%d|%d", facultyId,1);
            send(sock, request, sizeof(request), 0);

            // Step 1: Server will ask for old password
            read(sock, response, sizeof(response));
            printf("%s", response);  // prints "Enter old password: "
            char oldPass[30];
            scanf("%s", oldPass);
            send(sock, oldPass, sizeof(oldPass), 0);

            // Step 2: Server will respond (either "Incorrect password" or ask for new one)
            read(sock, response, sizeof(response));
            if (strncmp(response, "Incorrect password", 18) == 0) {
                printf("%s\n", response);  // just print the error
            } else {
                printf("%s", response); // prints "Enter new password: "
                char newPass[30];
                scanf("%s", newPass);
                send(sock, newPass, sizeof(newPass), 0);

                // Step 3: Final confirmation from server
                read(sock, response, sizeof(response));
                printf("%s\n", response);
            }
            break;

            default:
            printf("Invalid choice\n");
        }
        send(sock, request, sizeof(request), 0);
        
        int bytes_received = read(sock, response, sizeof(response) - 1);  // Leave space for null terminator
        if (bytes_received > 0) {
            response[bytes_received] = '\0';  // Null-terminate the received data
            printf("Server response: %s\n", response);
        } else if (bytes_received == 0) {
            printf("Server closed the connection\n");
        } else {
            perror("Error reading from socket");
        }
    }
    
    
}

void studentClient(int sock,int rollNum){
    printf(".......................Welcome to Student menu.....................\n");
    int studentid;
    while(1){
        char request[1024];
        char response[1024];
        printf("Do you want to:\n1. Enroll new Course\n2. Unenroll from already enrolled Courses\n3. View enrolled Courses\n4. Password Change\n5. Exit\n");
        printf("Enter your choice:\n");
        int choice;
        scanf("%d",&choice);

        if(choice==5)break;
        switch(choice){
            case 1:
            int facultyId;
            int courseId;
            printf("Enter faculty Id");
            scanf("%d",&facultyId);
            printf("Enter course id: ");
            scanf("%d", &courseId);
            sprintf(request,"ENROLL_COURSE|%d|%d|%d",rollNum,facultyId,courseId);
            break;

            case 2:
            facultyId;
            courseId;
            printf("Enter faculty Id");
            scanf("%d",&facultyId);
            printf("Enter course id: ");
            scanf("%d", &courseId);
            sprintf(request,"UNENROLL_COURSE|%d|%d|%d",rollNum,facultyId,courseId);
            break;

            case 3:
            sprintf(request,"VIEW_ENCOURSES|%d",rollNum);
            break;

            case 4:
            sprintf(request, "CHANGE_PASS|%d|%d", rollNum,2);  // 2 for student
            send(sock, request, sizeof(request), 0);

            // Step 1: Server asks for old password
            read(sock, response, sizeof(response));
            printf("%s", response);  // "Enter old password: "
            char oldPass[30];
            scanf("%s", oldPass);
            send(sock, oldPass, sizeof(oldPass), 0);

            // Step 2: Server replies — incorrect password or ask for new password
            read(sock, response, sizeof(response));
            if (strncmp(response, "Incorrect password", 18) == 0) {
                printf("%s\n", response);
            } else {
                printf("%s", response);  // "Enter new password: "
                char newPass[30];
                scanf("%s", newPass);
                send(sock, newPass, sizeof(newPass), 0);

                // Step 3: Final confirmation
                read(sock, response, sizeof(response));
                printf("%s\n", response);
            }
            break;


            default:
            printf("Invalid choice\n");
        }
        send(sock, request, sizeof(request), 0);
        int bytes_received = read(sock, response, sizeof(response) - 1);  // Leave space for null terminator
        if (bytes_received > 0) {
            response[bytes_received] = '\0';  // Null-terminate the received data
            printf("Server response: %s\n", response);
        } else if (bytes_received == 0) {
            printf("Server closed the connection\n");
        } else {
            perror("Error reading from socket");
        }
    }
    
}

int main() {
    printf("...........................Welcome Back to Academia :: Course Registration...........................\n");
    printf("Login Type\nEnter your choice{1.Admin , 2.Professor , 3.Student}\n");
    
    int choice;
    scanf("%d", &choice);

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        exit(EXIT_FAILURE);
    }

    int id;
    char password[30];
    printf("Enter ID: ");
    scanf("%d", &id);
    printf("Enter Password: ");
    scanf("%s", password);

    // Format: login_type:id:password
    char loginRequest[100];
    sprintf(loginRequest, "%d:%d:%s", choice, id, password);
    send(sock, loginRequest, strlen(loginRequest), 0);

    // Receive authentication result
    read(sock, buffer, sizeof(buffer));
    if (strcmp(buffer, "AUTH_SUCCESS") == 0) {
        printf("Login successful!\n");
        switch(choice) {
            case 1: adminClient(sock); break;
            case 2: facultyClient(sock, id); break;
            case 3: studentClient(sock, id); break;
            default: printf("Unknown role\n");
        }
    } else {
        printf("Authentication failed: %s\n", buffer);
    }

    close(sock);
    return 0;
}
