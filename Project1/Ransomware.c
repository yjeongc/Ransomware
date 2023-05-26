#define _CRT_SECURE_NO_WARNINGS //경고창이 뜨지 않도록 해줌.

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
            filename[i] = '_'; // 잘못된 문자를 밑줄(_)로 대체합니다.
        }
    }
}

int main() {

    //파일을 우선 압축한다
    char strOldFolder[] = "example.hacker_bye";  //strOldFolder에 압축하고 싶은 파일 경로를 넣는다.
    char strNewFolder[] = "example.hacker_bye.zip"; //strNewFolder에 압축한 파일의 이름을 넣는다.
    rename(strOldFolder, strNewFolder);


    //압축한 파일을 넣을 폴더 생성
    char strFolderPath[] = { "NewFolder" };

    int nResult = _mkdir(strFolderPath);

    if (nResult == 0)
    {
        printf("폴더 생성 성공\n");
    }
    else if (nResult == -1)
    {
        perror("폴더 생성 실패 - 폴더가 이미 있거나 부정확함\n");
        printf("errorno : %d\n", errno);
    }


    //압축해제
    unzFile zip_file;
    unz_file_info zip_info;
    char filename[256];
    int ret;
    int flag = 0;

    // ZIP 파일 열기
    zip_file = unzOpen("example.hacker_bye.zip");
    if (zip_file == NULL) {
        printf("failed to open example.hacker_bye.zip\n");
        exit(1);
    }

    // ZIP 파일 내용 읽기
    ret = unzGoToFirstFile(zip_file);
    while (ret == UNZ_OK) {
        // 파일 정보 가져오기
        ret = unzGetCurrentFileInfo(zip_file, &zip_info, filename, sizeof(filename), NULL, 0, NULL, 0);
        if (ret != UNZ_OK) {
            printf("failed to get file info\n");
            exit(1);
        }

        // 파일 추출
        ret = unzOpenCurrentFile(zip_file);
        if (ret != UNZ_OK) {
            printf("failed to open file\n");
            exit(1);
        }

        printf("[+]%s\n", filename);

        // 디렉터리 항목 건너뛰기
        if (filename[strlen(filename) - 1] == '/') {
            printf("Skipping directory entry: %s\n", filename);
            ret = unzGoToNextFile(zip_file);
            continue;
        }

        // 파일 이름 정리
        sanitize_filename(filename);

        // 새 경로로 파일 생성
        char new_filepath[256];
        snprintf(new_filepath, sizeof(new_filepath), "NewFolder/%s", filename);
        // 파일을 위한 디렉터리 생성
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

        // 다음 파일로 이동
        ret = unzGoToNextFile(zip_file);
    }

    // ZIP 파일 닫기
    unzClose(zip_file);

    //xml 문서 파싱하기
    xmlDocPtr doc;      // XML 문서 포인터
    xmlNodePtr root, node;  // XML 루트 요소 포인터, XML 요소 포인터
    const char* filepath = "NewFolder\\word_document.xml";  // 열 XML 파일 경로

    // XML 파일 열기
    //xml 파일을 파싱하여 메모리 상에 DOM 트리를 생성하는 역할을 한다.
    doc = xmlReadFile(filepath, NULL, 0);
    if (doc == NULL) {
        // 파일을 열지 못한 경우 오류 메시지 출력하고 종료
        fprintf(stderr, "Error: failed to parse %s\n", filepath);
        return 1;
    }

    // 루트 요소 가져오기 루트 요소를 가져오지 않으면 다음 노드 내용을 못가져오기 때문
    root = xmlDocGetRootElement(doc);
    if (root == NULL) {
        // 루트 요소가 없는 경우 오류 메시지 출력하고 메모리 해제 후 종료
        fprintf(stderr, "Error: empty document %s\n", filepath);
        xmlFreeDoc(doc);
        return 1;
    }

    // 모든 요소 순회하며 텍스트 내용 출력
    for (node = root; node; node = node->next) {
        if (node->type == XML_ELEMENT_NODE) {
            // 현재 요소가 XML 요소인 경우
            xmlChar* content = xmlNodeGetContent(node);  // 현재 요소의 텍스트 내용 가져오기
            printf("%s\n", content);  // 텍스트 내용 출력
            xmlFree(content);  // 텍스트 내용 메모리 해제
        }
    }

    // XML 파일 닫기
    xmlFreeDoc(doc);

    return 0;
}