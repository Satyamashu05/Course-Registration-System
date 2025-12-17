#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#define PORT 8080
#define MAX_BUFFER 1024
#define adminId 1234
#define adminPassword "7565"

typedef struct {
    int id;
    char name[30];
    char password[30];
    int active; // 1 = active, 0 = inactive
} student;

typedef struct {
    int id;
    char name[30];
    char password[30];
} faculty;

typedef struct {
    int facultyId;
    int courseId;
    char name[30];
    int credits;
    int numEnroll;
} course;

int lock_file(int fd, int type) {
    struct flock lock;
    lock.l_type = type;       // F_RDLCK or F_WRLCK
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;           // 0 = lock entire file
    lock.l_pid = getpid();
    return fcntl(fd, F_SETLKW, &lock); // wait if lock not available
}
void unlock_file(int fd) {
    struct flock lock;
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();
    fcntl(fd, F_SETLK, &lock);
}


int authenticate(int role, int id, char* password) {
    if (password == NULL) {
        return 0;
    }

    char *filename = NULL;

    // Admin check
    if (role == 1) {
        if (id == adminId && strcmp(password, adminPassword) == 0) {
            return 1;
        }
        return 0;
    }
    // Faculty file
    else if (role == 2) {
        filename = "faculty.txt";
    }
    // Student file
    else if (role == 3) {
        filename = "students.txt";
    }
    // Invalid role
    else {
        return 0;
    }

    // Open file (syscall)
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open file");
        return 0;
    }

    char buffer[1024];
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1); // Leave space for null terminator
    close(fd); // Close file immediately after reading

    if (bytes_read <= 0) {
        return 0;
    }

    buffer[bytes_read] = '\0'; // Null-terminate the buffer

    char *line = strtok(buffer, "\n");
    while (line != NULL) {
        int file_id;
        char file_pass[30];

        // Parse line (format: ID Name Password)
        if (sscanf(line, "%d %*s %29s", &file_id, file_pass) == 2) {
            if (file_id == id && strcmp(file_pass, password) == 0) {
                return 1; // Success
            }
        }
        line = strtok(NULL, "\n");
    }

    return 0; // Failure
}

void handle_client(int client_sock) {
    char buffer[1024];
    while (1) {
        int r = read(client_sock, buffer, sizeof(buffer));
        if (r <= 0) break;
        buffer[r] = '\0';

        char *cmd = strtok(buffer, "|");

        if (strcmp(cmd, "ADD_STUDENT") == 0) {
            char *id_str = strtok(NULL, "|");
            char *name = strtok(NULL, "|");
            char *pass = strtok(NULL, "|");

            if (!id_str || !name || !pass) {
                write(client_sock, "ERROR: Invalid format. Use: ADD_STUDENT|id|name|password\n", 54);
                return;
            }

            student s;
            s.id = atoi(id_str);
            strcpy(s.name, name);
            strcpy(s.password, pass);
            s.active = 0;

            // Open file (create if it doesn't exist) in append mode
            int fd = open("students.txt", O_WRONLY | O_APPEND | O_CREAT, 0644);
            if (fd == -1) {
                perror("Error opening file");
                write(client_sock, "ERROR: Could not save student\n", 30);
                return;
            }

            // Format student data as text
            char buffer[256];
            int len = snprintf(buffer, sizeof(buffer), "%d %s %s %d\n", s.id, s.name, s.password, s.active);

            // Write to file
            if (write(fd, buffer, len) != len) {
                perror("Error writing to file");
                write(client_sock, "ERROR: Failed to write student data\n", 35);
            } else {
                write(client_sock, "SUCCESS: Student added\n", 23);
            }
            close(fd);
        }
        else if (strcmp(cmd, "ADD_FACULTY") == 0) {
            char *id_str = strtok(NULL, "|");
            char *name = strtok(NULL, "|");
            char *pass = strtok(NULL, "|");
            
            if (!id_str || !name || !pass) {
                write(client_sock, "ERROR: Invalid format. Use: ADD_FACULTY|id|name|password\n", 54);
                return;
            }

            faculty f ;
            f.id=atoi(id_str);
            strncpy(f.name, name, sizeof(f.name) - 1);
            strncpy(f.password, pass, sizeof(f.password) - 1);

            int fd = open("faculty.txt", O_WRONLY | O_APPEND | O_CREAT, 0644);
            if (fd == -1) {
                perror("Error opening file");
                write(client_sock, "ERROR: Could not save faculty\n", 30);
                return;
            }

            // Format student data as text
            char buffer[256];
            int len = snprintf(buffer, sizeof(buffer), "%d %s %s\n", f.id, f.name, f.password);

            // Write to file
            if (write(fd, buffer, len) != len) {
                perror("Error writing to file");
                write(client_sock, "ERROR: Failed to write student data\n", 35);
            } else {
                write(client_sock, "SUCCESS: Faculty added\n", 23);
            }
            close(fd);
        }
        else if (strcmp(cmd, "ADD_COURSE") == 0) {
            int facultyId = atoi(strtok(NULL,"|"));
            int courseid = atoi(strtok(NULL, "|"));
            char *name = strtok(NULL, "|");
            int credits = atoi(strtok(NULL, "|"));

            course c;
            c.facultyId=facultyId,c.courseId=courseid;
            strcpy(c.name, name);
            c.credits = credits;
            c.numEnroll = 0;

            int fd = open("courses.txt", O_WRONLY | O_APPEND | O_CREAT, 0644);

            if (fd == -1) {
                perror("Error opening file");
                write(client_sock, "ERROR: Could not save faculty\n", 30);
                return;
            }

            // Apply write lock to enrollment.txt
            if (lock_file(fd, F_WRLCK) < 0) {
                write(client_sock, "Unable to lock enrollment.txt\n", 30);
                close(fd);
                continue;
            }

            // Format student data as text
            char buffer[256];
            int len = snprintf(buffer, sizeof(buffer), "%d %d %s %d %d\n",c.facultyId,c.courseId,c.name,c.credits,c.numEnroll);

            // Write to file
            if (write(fd, buffer, len) != len) {
                perror("Error writing to file");
                write(client_sock, "ERROR: Failed to write student data\n", 35);
            } else {
                write(client_sock, "SUCCESS: Course added\n", 23);
            }
            unlock_file(fd);
            close(fd);
        }
        else if (strcmp(cmd, "ACTIVATE_STUDENT") == 0 || strcmp(cmd, "DEACTIVATE_STUDENT") == 0) {
            char *id_str = strtok(NULL, "|");
            if (!id_str) {
                write(client_sock, "ERROR: Missing student ID\n", 25);
                return;
            }
            int id = atoi(id_str);
            int found = 0;
        
            // Open file for reading and writing
            int fd = open("students.txt", O_RDWR);
            if (fd == -1) {
                write(client_sock, "ERROR: Could not open student file\n", 34);
                return;
            }
        
            // Read entire file
            char buffer[4096];
            ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
            if (bytes_read <= 0) {
                close(fd);
                write(client_sock, "ERROR: Empty student file\n", 26);
                return;
            }
            buffer[bytes_read] = '\0';
        
            // Process content
            char updated_content[4096] = {0};
            char *saveptr; // Declare saveptr for strtok_r
            char *line = strtok_r(buffer, "\n", &saveptr);
            
            while (line != NULL) {
                char line_copy[256];
                strncpy(line_copy, line, sizeof(line_copy) - 1);
                
                char *token = strtok(line_copy, " ");
                printf("roll number is %s\n",token);
                if (token && atoi(token) == id) {
                    found = 1;
                    // Parse all fields
                    char *name = strtok(NULL, " ");
                    char *pass = strtok(NULL, " ");
                    char *status = strtok(NULL, " ");
                    
                    // Create updated line
                    char updated_line[256];
                    snprintf(updated_line, sizeof(updated_line), "%d %s %s %d\n", 
                            id,name,pass,(strcmp(cmd, "ACTIVATE_STUDENT") == 0) ? 1 : 0);                    
                    strcat(updated_content, updated_line);
                } else {
                    // Preserve original line
                    strcat(updated_content, line);
                    strcat(updated_content, "\n");
                }
                line = strtok_r(NULL, "\n", &saveptr);
            }
        
            // Write back changes if student was found
            if (found) {
                ftruncate(fd, 0);
                lseek(fd, 0, SEEK_SET);
                if (write(fd, updated_content, strlen(updated_content)) == -1) {
                    perror("Error writing file");
                }
            }
            close(fd);
        
            // Send response
            char response[128];
            snprintf(response, sizeof(response), 
                    found ? "SUCCESS: Student %d %s\n" : "ERROR: Student %d not found\n",
                    id,
                    found ? ((strcmp(cmd, "ACTIVATE_STUDENT") == 0) ? "activated" : "deactivated") : "");
            write(client_sock, response, strlen(response));
        }               
        else if (strcmp(cmd, "UPDATE_STUDENT") == 0) {
            // Parse input: UPDATE_STUDENT|id|new_name
            char *id_str = strtok(NULL, "|");
            char *new_name = strtok(NULL, "|");
            
            if (!id_str || !new_name) {
                write(client_sock, "ERROR: Format should be UPDATE_STUDENT|id|new_name\n", 48);
                continue;
            }
            
            int id = atoi(id_str);
            int found = 0;
            
            // Open files
            int fd = open("students.txt", O_RDONLY);
            int temp_fd = open("temp.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
            
            if (fd < 0 || temp_fd < 0) {
                write(client_sock, "ERROR: Could not open files\n", 27);
                if (fd >= 0) close(fd);
                if (temp_fd >= 0) close(temp_fd);
                continue;
            }
            
            // Process file
            char buffer[1024];
            ssize_t bytes_read;
            
            while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
                char *saveptr;
                char *line = strtok_r(buffer, "\n", &saveptr);
                
                while (line != NULL) {
                    char line_copy[256];
                    strncpy(line_copy, line, sizeof(line_copy) - 1);
                    
                    // Parse fields: id name password active
                    char *token = strtok(line_copy, " ");
                    if (token) {
                        int current_id = atoi(token);
                        char *current_name = strtok(NULL, " ");
                        char *current_pass = strtok(NULL, " ");
                        char *current_active = strtok(NULL, " ");
                        
                        if (current_id == id) {
                            dprintf(temp_fd, "%d %s %s %s\n", 
                                   current_id, new_name, current_pass ? current_pass : "", 
                                   current_active ? current_active : "0");
                            found = 1;
                        } else {
                            dprintf(temp_fd, "%s\n", line);
                        }
                    }
                    line = strtok_r(NULL, "\n", &saveptr);
                }
            }
            
            close(fd);
            close(temp_fd);
            
            // Handle result
            if (found) {
                unlink("students.txt");
                rename("temp.txt", "students.txt");
                write(client_sock, "SUCCESS: Student name updated\n", 29);
            } else {
                unlink("temp.txt");
                write(client_sock, "ERROR: Student ID not found\n", 27);
            }
        }
        else if (strcmp(cmd, "UPDATE_FACULTY") == 0) {
            // Parse input: UPDATE_STUDENT|id|new_name
            char *id_str = strtok(NULL, "|");
            char *new_name = strtok(NULL, "|");
            
            if (!id_str || !new_name) {
                write(client_sock, "ERROR: Format should be UPDATE_FACULTY|id|new_name\n", 48);
                continue;
            }
            
            int id = atoi(id_str);
            int found = 0;
            
            // Open files
            int fd = open("faculty.txt", O_RDONLY);
            int temp_fd = open("temp.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
            
            if (fd < 0 || temp_fd < 0) {
                write(client_sock, "ERROR: Could not open files\n", 27);
                if (fd >= 0) close(fd);
                if (temp_fd >= 0) close(temp_fd);
                continue;
            }
            
            // Process file
            char buffer[1024];
            ssize_t bytes_read;
            
            while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
                char *saveptr;
                char *line = strtok_r(buffer, "\n", &saveptr);
                
                while (line != NULL) {
                    char line_copy[256];
                    strncpy(line_copy, line, sizeof(line_copy) - 1);
                    
                    // Parse fields: id|name|password|active
                    char *token = strtok(line_copy, " ");
                    if (token) {
                        int current_id = atoi(token);
                        char *current_name = strtok(NULL, " ");
                        char *current_pass = strtok(NULL, " ");
                        
                        if (current_id == id) {
                            dprintf(temp_fd, "%d %s %s\n", 
                                   current_id, new_name, current_pass ? current_pass : "");
                            found = 1;
                        } else {
                            dprintf(temp_fd, "%s\n", line);
                        }
                    }
                    line = strtok_r(NULL, "\n", &saveptr);
                }
            }
            
            close(fd);
            close(temp_fd);
            
            // Handle result
            if (found) {
                unlink("faculty.txt");
                rename("temp.txt", "faculty.txt");
                write(client_sock, "SUCCESS: faculty name updated\n", 29);
            } else {
                unlink("temp.txt");
                write(client_sock, "ERROR: Faculty ID not found\n", 27);
            }
        }
        else if (strcmp(cmd, "REMOVE_COURSE") == 0) {
            // Parse input: REMOVE_COURSE|facultyId|courseId
            char *facultyId_str = strtok(NULL, "|");
            char *courseId_str = strtok(NULL, "|");
            
            if (!facultyId_str || !courseId_str) {
                write(client_sock, "ERROR: Format should be REMOVE_COURSE|facultyId|courseId\n", 53);
                continue;
            }
            
            int facultyId = atoi(facultyId_str);
            int courseId = atoi(courseId_str);
            int found = 0;
            
            // Open files
            int fd = open("courses.txt", O_RDONLY);
            int temp_fd = open("temp.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
            
            if (fd < 0 || temp_fd < 0) {
                write(client_sock, "ERROR: Could not open files\n", 27);
                if (fd >= 0) close(fd);
                if (temp_fd >= 0) close(temp_fd);
                continue;
            }

            // Apply write lock to enrollment.txt
            if (lock_file(fd, F_WRLCK) < 0) {
                write(client_sock, "Unable to lock enrollment.txt\n", 30);
                close(fd);
                close(temp_fd);
                continue;
            }
            
            // Process file
            char buffer[1024];
            ssize_t bytes_read;
            
            while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
                char *saveptr;
                char *line = strtok_r(buffer, "\n", &saveptr);
                
                while (line != NULL) {
                    char line_copy[256];
                    strncpy(line_copy, line, sizeof(line_copy) - 1);
                    
                    // Parse fields: courseId|facultyId|name|credit|studentCount|maxSeats
                    char *token = strtok(line_copy, " ");
                    if (token) {
                        int current_facultyId = atoi(token);
                        token = strtok(NULL, " ");
                        int current_courseId = atoi(token);
                        //printf("%d %d\n",current_facultyId,current_courseId);
                        if (current_courseId == courseId && current_facultyId == facultyId) {
                            found = 1; // skip writing this line
                        } else {
                            dprintf(temp_fd, "%s\n", line);
                        }
                    }
                    line = strtok_r(NULL, "\n", &saveptr);
                }
            }
            
            close(fd);
            close(temp_fd);
            
            // Handle result
            if (found) {
                unlink("courses.txt");
                rename("temp.txt", "courses.txt");
                write(client_sock, "SUCCESS: Course removed successfully\n", 35);
            } else {
                unlink("temp.txt");
                write(client_sock, "ERROR: Course not found\n", 23);
            }
            unlock_file(fd);
        }else if (strcmp(cmd, "VIEW_ENROLLMENTS") == 0) {
            // Parse input: VIEW_ENROLLMENTS|facultyId
            char *facultyId_str = strtok(NULL, "|");
            if (!facultyId_str) {
                write(client_sock, "ERROR: Format should be VIEW_ENROLLMENTS|facultyId\n", 50);
                continue;
            }
            
            int facultyId = atoi(facultyId_str);
            int found = 0;
            
            // Open file
            int fd = open("courses.txt", O_RDONLY);
            if (fd < 0) {
                write(client_sock, "ERROR: Could not open courses file\n", 34);
                continue;
            }
            // Apply write lock to enrollment.txt
            if (lock_file(fd, F_WRLCK) < 0) {
                write(client_sock, "Unable to lock enrollment.txt\n", 30);
                close(fd);
                continue;
            }
            // Process file and build response
            char buffer[1024];
            char response[2048] = "Enrolled Courses:\n";
            ssize_t bytes_read;
            while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
                char *saveptr;
                char *line = strtok_r(buffer, "\n", &saveptr);
               // printf("%s\n",line);
                while (line != NULL) {
                  //  printf("%s\n",line);
                    char line_copy[256];
                    strncpy(line_copy, line, sizeof(line_copy) - 1);
                    
                    // Parse fields: facultyId courseId coursename credits numEnrollments (space separated)
                    char *token = strtok(line_copy, " ");
                    if (token) {
                        int current_facultyId = atoi(token);
                        
                        if (current_facultyId == facultyId) {
                            found = 1;
                            token = strtok(NULL, " "); // courseId
                            int courseId = atoi(token);
                            token = strtok(NULL, " "); // coursename
                            char *coursename = token;
                            token = strtok(NULL, " "); // credits
                            token = strtok(NULL, " "); // numEnrollments
                            int numEnroll = atoi(token);
                            
                            char line_response[128];
                            snprintf(line_response, sizeof(line_response), 
                                    "Course ID: %d | Name: %s | Enrollments: %d\n", 
                                    courseId, coursename, numEnroll);
                            strncat(response, line_response, sizeof(response) - strlen(response) - 1);
                        }
                    }
                    line = strtok_r(NULL, "\n", &saveptr);
                }
            }
            
            close(fd);
            
            if (!found) {
                strncpy(response, "No courses found for this faculty.\n", sizeof(response));
            }            
            write(client_sock, response, strlen(response));
            unlock_file(fd);
        }else if (strcmp(cmd, "CHANGE_PASS") == 0) {
            // Parse input: CHANGE_PASS|id|person (1=faculty, 2=student)
            char *id_str = strtok(NULL, "|");
            char *person_str = strtok(NULL, "|");
            
            if (!id_str || !person_str) {
                write(client_sock, "ERROR: Format should be CHANGE_PASS|id|person\n", 45);
                continue;
            }
            
            int id = atoi(id_str);
            int person = atoi(person_str);
            char *filename = (person == 1) ? "faculty.txt" : "students.txt";
            
            // Open files
            int fd = open(filename, O_RDONLY);
            int temp_fd = open("temp.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
            
            if (fd < 0 || temp_fd < 0) {
                write(client_sock, "ERROR: Could not open files\n", 27);
                if (fd >= 0) close(fd);
                if (temp_fd >= 0) close(temp_fd);
                continue;
            }
            // Apply write lock to enrollment.txt
            // if (lock_file(fd, F_WRLCK) < 0) {
            //     write(client_sock, "Unable to lock enrollment.txt\n", 30);
            //     close(fd);
            //     close(temp_fd);
            //     continue;
            // }
            // Ask for old password
            write(client_sock, "Enter old password: ", 21);
            char old_pass[50];
            int n = read(client_sock, old_pass, sizeof(old_pass));
            if (n <= 0) {
                close(fd);
                close(temp_fd);
                unlink("temp.txt");
                continue;
            }
            old_pass[n] = '\0';
            old_pass[strcspn(old_pass, "\n")] = '\0';
            
            // Process file
            char buffer[1024];
            ssize_t bytes_read;
            int found = 0;
            int password_matched = 0;
            
            while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
                char *saveptr;
                char *line = strtok_r(buffer, "\n", &saveptr);
                
                while (line != NULL) {
                    char line_copy[256];
                    strncpy(line_copy, line, sizeof(line_copy) - 1);
                    
                    // Parse fields (faculty: id|name|password|active, student: id|name|password|email|active)
                    char *token = strtok(line_copy, " ");
                    if (token) {
                        int current_id = atoi(token);
                        
                        if (current_id == id) {
                            found = 1;
                            token = strtok(NULL, " "); // name
                            char *current_name = token;
                            token = strtok(NULL, " "); // password
                            char *current_pass = token;
                            
                            if (strcmp(current_pass, old_pass) == 0) {
                                password_matched = 1;
                                
                                // Ask for new password
                                write(client_sock, "Enter new password: ", 21);
                                char new_pass[50];
                                n = read(client_sock, new_pass, sizeof(new_pass));
                                if (n <= 0) {
                                    close(fd);
                                    close(temp_fd);
                                    unlink("temp.txt");
                                    continue;
                                }
                                new_pass[n] = '\0';
                                new_pass[strcspn(new_pass, "\n")] = '\0';
                                
                                // Write updated record
                                if (person == 1) {
                                    dprintf(temp_fd, "%d %s %s\n", current_id, current_name, new_pass);
                                } else {
                                    token = strtok(NULL, " "); // email
                                    char *email = token;
                                    dprintf(temp_fd, "%d %s %s %s\n", current_id, current_name, new_pass, email);
                                }
                            } else {
                                // Write original record (password didn't match)
                                dprintf(temp_fd, "%s\n", line);
                            }
                        } else {
                            // Write other records unchanged
                            dprintf(temp_fd, "%s\n", line);
                        }
                    }
                    line = strtok_r(NULL, "\n", &saveptr);
                }
            }
            
            close(fd);
            close(temp_fd);
            
            // Handle result
            if (!found) {
                unlink("temp.txt");
                write(client_sock, "ERROR: ID not found\n", 20);
            } else if (!password_matched) {
                unlink("temp.txt");
                write(client_sock, "ERROR: Incorrect password\n", 26);
            } else {
                unlink(filename);
                rename("temp.txt", filename);
                write(client_sock, "SUCCESS: Password updated\n", 26);
            }

            //unlock_file(fd);
        }                  
        else if (strcmp(cmd, "ENROLL_COURSE") == 0) {
            // Parse input: ENROLL_COURSE|rollNum|facultyId|courseId
            char *rollNum_str = strtok(NULL, "|");
            char *facultyId_str = strtok(NULL, "|");
            char *courseId_str = strtok(NULL, "|");
            
            if (!rollNum_str || !facultyId_str || !courseId_str) {
                write(client_sock, "ERROR: Format should be ENROLL_COURSE|rollNum|facultyId|courseId\n", 60);
                continue;
            }
            
            int rollNum = atoi(rollNum_str);
            int facultyId = atoi(facultyId_str);
            int courseId = atoi(courseId_str);
            
            // 1. Update enrollment.txt (text file)
            int enroll_fd = open("enrollment.txt", O_RDONLY | O_CREAT, 0644);
            int temp_fd = open("enrollment.tmp", O_WRONLY | O_CREAT | O_TRUNC, 0644);
            
            if (enroll_fd < 0 || temp_fd < 0) {
                write(client_sock, "ERROR: Could not open enrollment files\n", 38);
                if (enroll_fd >= 0) close(enroll_fd);
                if (temp_fd >= 0) close(temp_fd);
                continue;
            }
            
            // // Apply write lock to enrollment.txt
            // if (lock_file(enroll_fd, F_WRLCK) < 0) {
            //     write(client_sock, "Unable to lock enrollment.txt\n", 30);
            //     close(enroll_fd);
            //     close(temp_fd);
            //     continue;
            // }

            char buf[1024];
            ssize_t bytes_read;
            int found_enrollment = 0;
            
            while ((bytes_read = read(enroll_fd, buf, sizeof(buf)-1)) > 0) {
                buf[bytes_read] = '\0';
                char *line = strtok(buf, "\n");
                while (line) {
                    int current_roll;
                    if (sscanf(line, "%d", &current_roll) == 1 && current_roll == rollNum) {
                        dprintf(temp_fd, "%s %d\n", line, courseId);
                        found_enrollment = 1;
                    } else {
                        dprintf(temp_fd, "%s\n", line);
                    }
                    line = strtok(NULL, "\n");
                }
            }
            
            if (!found_enrollment) {
                dprintf(temp_fd, "%d %d\n", rollNum, courseId);
            }
            
            close(enroll_fd);
            close(temp_fd);
            unlink("enrollment.txt");
            rename("enrollment.tmp", "enrollment.txt");
            
            // 2. Update courses.txt (text file)
            int courses_fd = open("courses.txt", O_RDONLY);
            int courses_temp_fd = open("courses.tmp", O_WRONLY | O_CREAT | O_TRUNC, 0644);
            
            if (courses_fd < 0 || courses_temp_fd < 0) {
                write(client_sock, "ERROR: Could not open courses files\n", 35);
                if (courses_fd >= 0) close(courses_fd);
                if (courses_temp_fd >= 0) close(courses_temp_fd);
                continue;
            }
            
            int found_course = 0;
            
            while ((bytes_read = read(courses_fd, buf, sizeof(buf)-1)) > 0) {
                buf[bytes_read] = '\0';
                char *line = strtok(buf, "\n");
                while (line) {
                    char line_copy[256];
                    strncpy(line_copy, line, sizeof(line_copy)-1);
                    
                    // Parse: facultyId courseId name credits numEnroll
                    int current_facultyId, current_courseId, credits, numEnroll;
                    char name[30];
                    
                    if (sscanf(line_copy, "%d %d %29s %d %d", 
                               &current_facultyId, &current_courseId, name, &credits, &numEnroll) == 5) {
                        
                        if (current_courseId == courseId && current_facultyId == facultyId) {
                            dprintf(courses_temp_fd, "%d %d %s %d %d\n", 
                                   current_facultyId, current_courseId, name, credits, numEnroll+1);
                            found_course = 1;
                        } else {
                            dprintf(courses_temp_fd, "%s\n", line);
                        }
                    } else {
                        dprintf(courses_temp_fd, "%s\n", line);
                    }
                    line = strtok(NULL, "\n");
                }
            }
            
            close(courses_fd);
            close(courses_temp_fd);
            
            if (found_course) {
                unlink("courses.txt");
                rename("courses.tmp", "courses.txt");
                write(client_sock, "SUCCESS: Enrollment completed\n", 29);
            } else {
                unlink("courses.tmp");
                write(client_sock, "ERROR: Course not found\n", 24);
            }
            //unlock_file(enroll_fd);
        }else if (strcmp(cmd, "UNENROLL_COURSE") == 0) {
            int rollNum   = atoi(strtok(NULL, "|"));
            int facultyId = atoi(strtok(NULL, "|"));
            int courseId  = atoi(strtok(NULL, "|"));
        
            // === STEP 1: Modify enrollment.txt using only syscalls ===
            int in_fd  = open("enrollment.txt", O_RDONLY | O_CREAT, 0644);
            int out_fd = open("enrollment.tmp", O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (in_fd < 0 || out_fd < 0) {
                write(client_sock, "Error accessing enrollment file\n", 32);
                if (in_fd >= 0) close(in_fd);
                if (out_fd >= 0) close(out_fd);
                continue;
            }
            
            // Apply write lock to enrollment.txt
            if (lock_file(in_fd, F_WRLCK) < 0) {
                write(client_sock, "Unable to lock enrollment.txt\n", 30);
                close(in_fd);
                close(out_fd);
                continue;
            }

            char buf[1024], line[1024];
            ssize_t n;
            int i = 0, found = 0;
            while ((n = read(in_fd, buf, sizeof(buf))) > 0) {
                for (ssize_t j = 0; j < n; ++j) {
                    if (buf[j] == '\n') {
                        line[i] = '\0';
                        i = 0;
        
                        int id;
                        char newline[1024] = "";
                        char temp[1024];
                        strcpy(temp, line);
                        char *token = strtok(temp, " ");
                        if (!token) continue;
                        id = atoi(token);
                        sprintf(newline, "%d", id);
        
                        if (id == rollNum) {
                            token = strtok(NULL, " ");
                            while (token) {
                                int cid = atoi(token);
                                if (cid == courseId) {
                                    found = 1;
                                    // skip writing this courseId
                                } else {
                                    strcat(newline, " ");
                                    strcat(newline, token);
                                }
                                token = strtok(NULL, " ");
                            }
                            strcat(newline, "\n");
                            write(out_fd, newline, strlen(newline));
                        } else {
                            strcat(line, "\n");
                            write(out_fd, line, strlen(line));
                        }
                    } else {
                        line[i++] = buf[j];
                    }
                }
            }
        
            close(in_fd);
            close(out_fd);
            unlink("enrollment.txt");
            rename("enrollment.tmp", "enrollment.txt");
            unlock_file(in_fd);
            // === STEP 2: Update numEnroll in courses.txt (with syscalls only) ===
            int c_fd = open("courses.txt", O_RDWR);
            if (c_fd < 0) {
                write(client_sock, "Error opening courses file\n", 28);
                continue;
            }
            
            // Apply write lock to enrollment.txt
            if (lock_file(c_fd, F_WRLCK) < 0) {
                write(client_sock, "Unable to lock enrollment.txt\n", 30);
                close(c_fd);
                continue;
            }
            struct course {
                int facultyId;
                int courseId;
                char name[30];
                int credits;
                int numEnroll;
            } c;
        
            off_t offset = 0;
            int courseFound = 0;
            while (read(c_fd, &c, sizeof(c)) == sizeof(c)) {
                if (c.courseId == courseId && c.facultyId == facultyId) {
                    if (c.numEnroll > 0) c.numEnroll--;
                    lseek(c_fd, offset, SEEK_SET);
                    write(c_fd, &c, sizeof(c));
                    courseFound = 1;
                    break;
                }
                offset += sizeof(c);
            }
        
            close(c_fd);
            unlock_file(c_fd);
            // === Final response to client ===
            if (!found) {
                write(client_sock, "Enrollment record not found\n", 29);
            } else if (!courseFound) {
                write(client_sock, "Course not found\n", 17);
            } else {
                write(client_sock, "Unenrollment successful\n", 24);
            }
        }               
        else if (strcmp(cmd, "VIEW_ENCOURSES") == 0) {
            int rollNum = atoi(strtok(NULL, "|"));
        
            // Read entire enrollment.txt into a buffer
            int fd = open("enrollment.txt", O_RDONLY | O_CREAT, 0644);
            if (fd < 0) {
                write(client_sock, "Error opening enrollment file\n", 31);
                continue;
            }

            // Apply write lock to enrollment.txt
            if (lock_file(fd, F_RDLCK) < 0) {
                write(client_sock, "Unable to lock enrollment.txt\n", 30);
                close(fd);
                continue;
            }

            char buf[4096];
            ssize_t n = read(fd, buf, sizeof(buf) - 1);
            close(fd);
            if (n <= 0) {
                write(client_sock, "No enrolled courses found\n", 26);
                continue;
            }
            buf[n] = '\0';
        
            // Find the line for this student
            char *line = strtok(buf, "\n");
            int found = 0;
            while (line) {
                int id;
                if (sscanf(line, "%d", &id) == 1 && id == rollNum) {
                    found = 1;
                    // Skip past the roll number and space
                    char *courses = strchr(line, ' ');
                    if (courses && *(courses + 1) != '\0') {
                        write(client_sock, courses + 1, strlen(courses + 1));
                        write(client_sock, "\n", 1);
                    } else {
                        write(client_sock, "No enrolled courses\n", 20);
                    }
                    break;
                }
                line = strtok(NULL, "\n");
            }
            unlock_file(fd);
            if (!found) {
                write(client_sock, "No enrolled courses found\n", 26);
            }
        }
                         
        else {
            write(client_sock, "Invalid command\n", 17);
        }
    }
    close(client_sock);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[MAX_BUFFER];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);
    printf("Server listening on port %d...\n", PORT);

    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if(client_socket<0){
            perror("Accept");
            continue;
        }

        memset(buffer, 0, sizeof(buffer));
        read(client_socket, buffer, sizeof(buffer));
        printf("Received login : %s\n", buffer);

        int role, id;char* password;
        sscanf(buffer, "%d:%d:%s", &role, &id, password);

        if (authenticate(role, id, password)) {
            write(client_socket, "AUTH_SUCCESS", strlen("AUTH_SUCCESS"));
        } else {
            write(client_socket, "AUTH_FAILURE: Invalid credentials", strlen("AUTH_FAILURE: Invalid credentials"));
            continue;
        }

        if (fork() == 0) {
            close(server_fd); // child doesn't need listener
            handle_client(client_socket);
            exit(0);
        }
        close(client_socket); // parent doesn't need this client socket
    }

    return 0;
}
