#define _CRT_SECURE_NO_WARNINGS //���â�� ���� �ʵ��� ����.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unzip.h>
#include <sys/stat.h>
#include <windows.h>
#include <direct.h>     //mkdir
#include <errno.h>      //errno
#include <libxml/parser.h>
#include <libxml/tree.h>


int create_directory_for_file(const char* filepath)
{
    int result = 0;
    char* path = NULL;
    const char* slash_pos = strrchr(filepath, '/');
    if (slash_pos != NULL)
    {
        int path_len = slash_pos - filepath + 1;
        path = (char*)malloc(path_len + 1);
        if (path != NULL)
        {
            strncpy(path, filepath, path_len);
            path[path_len] = '\0';
            while (path_len > 1 && path[path_len - 2] == '/')
            {
                path[path_len - 2] = '\0';
                path_len--;
            }
            if (CreateDirectoryA(path, NULL) || GetLastError() == ERROR_ALREADY_EXISTS)
            {
                result = -1;
            }
        }
        free(path);
    }
    return result;
}

void sanitize_filename(char* filename)
{
    const char* invalid_chars = "\\/:*?\"<>|";
    size_t len = strlen(filename);
    for (size_t i = 0; i < len; ++i)
    {
        if (strchr(invalid_chars, filename[i]) != NULL)
        {
            filename[i] = '_'; // �߸��� ���ڸ� ����(_)�� ��ü�մϴ�.
        }
    }
}

int main() {

    //������ �켱 �����Ѵ�
    char strOldFolder[] = "example.hacker_bye";  //strOldFolder�� �����ϰ� ���� ���� ��θ� �ִ´�.
    char strNewFolder[] = "example.hacker_bye.zip"; //strNewFolder�� ������ ������ �̸��� �ִ´�.
    rename(strOldFolder, strNewFolder);


    //������ ������ ���� ���� ����
    char strFolderPath[] = { "NewFolder" };

    int nResult = _mkdir(strFolderPath);

    if (nResult == 0)
    {
        printf("���� ���� ����\n");
    }
    else if (nResult == -1)
    {
        perror("���� ���� ���� - ������ �̹� �ְų� ����Ȯ��\n");
        printf("errorno : %d\n", errno);
    }


    //��������
    unzFile zip_file;
    unz_file_info zip_info;
    char filename[256];
    int ret;
    int flag = 0;

    // ZIP ���� ����
    zip_file = unzOpen("example.hacker_bye.zip");
    if (zip_file == NULL) {
        printf("failed to open example.hacker_bye.zip\n");
        exit(1);
    }

    // ZIP ���� ���� �б�
    ret = unzGoToFirstFile(zip_file);
    while (ret == UNZ_OK) {
        // ���� ���� ��������
        ret = unzGetCurrentFileInfo(zip_file, &zip_info, filename, sizeof(filename), NULL, 0, NULL, 0);
        if (ret != UNZ_OK) {
            printf("failed to get file info\n");
            exit(1);
        }

        // ���� ����
        ret = unzOpenCurrentFile(zip_file);
        if (ret != UNZ_OK) {
            printf("failed to open file\n");
            exit(1);
        }

        printf("[+]%s\n", filename);

        // ���͸� �׸� �ǳʶٱ�
        if (filename[strlen(filename) - 1] == '/') {
            printf("Skipping directory entry: %s\n", filename);
            ret = unzGoToNextFile(zip_file);
            continue;
        }

        // ���� �̸� ����
        sanitize_filename(filename);

        // �� ��η� ���� ����
        char new_filepath[256];
        snprintf(new_filepath, sizeof(new_filepath), "NewFolder/%s", filename);
        // ������ ���� ���͸� ����
        create_directory_for_file(new_filepath);

        FILE* out_file = fopen(new_filepath, "wb");
        if (out_file == NULL) {
            printf("failed to create file %s\n", new_filepath);
            exit(1);
        }

        char buffer[4096];
        int read_len;
        do {
            read_len = unzReadCurrentFile(zip_file, buffer, sizeof(buffer));
            if (read_len < 0) {
                printf("failed to read file\n");
                exit(1);
            }
            if (read_len > 0) {
                fwrite(buffer, read_len, 1, out_file);
            }
        } while (read_len > 0);

        fclose(out_file);
        unzCloseCurrentFile(zip_file);

        // ���� ���Ϸ� �̵�
        ret = unzGoToNextFile(zip_file);
    }

    // ZIP ���� �ݱ�
    unzClose(zip_file);

    //xml ���� �Ľ��ϱ�
    xmlDocPtr doc;      // XML ���� ������
    xmlNodePtr root, node;  // XML ��Ʈ ��� ������, XML ��� ������
    const char* filepath = "NewFolder\\word_document.xml";  // �� XML ���� ���

    // XML ���� ����
    //xml ������ �Ľ��Ͽ� �޸� �� DOM Ʈ���� �����ϴ� ������ �Ѵ�.
    doc = xmlReadFile(filepath, NULL, 0);
    if (doc == NULL) {
        // ������ ���� ���� ��� ���� �޽��� ����ϰ� ����
        fprintf(stderr, "Error: failed to parse %s\n", filepath);
        return 1;
    }

    // ��Ʈ ��� �������� ��Ʈ ��Ҹ� �������� ������ ���� ��� ������ ���������� ����
    root = xmlDocGetRootElement(doc);
    if (root == NULL) {
        // ��Ʈ ��Ұ� ���� ��� ���� �޽��� ����ϰ� �޸� ���� �� ����
        fprintf(stderr, "Error: empty document %s\n", filepath);
        xmlFreeDoc(doc);
        return 1;
    }

    // ��� ��� ��ȸ�ϸ� �ؽ�Ʈ ���� ���
    for (node = root; node; node = node->next) {
        if (node->type == XML_ELEMENT_NODE) {
            // ���� ��Ұ� XML ����� ���
            xmlChar* content = xmlNodeGetContent(node);  // ���� ����� �ؽ�Ʈ ���� ��������
            printf("%s\n", content);  // �ؽ�Ʈ ���� ���
            xmlFree(content);  // �ؽ�Ʈ ���� �޸� ����
        }
    }

    // XML ���� �ݱ�
    xmlFreeDoc(doc);

    return 0;
}