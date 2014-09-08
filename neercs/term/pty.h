//
// Neercs
//

#pragma once

class Pty
{
public:
    Pty();
    ~Pty();

    void Run(char const *command, ivec2 size);
    bool IsEof() const;

    size_t ReadData(char *data, size_t maxlen);
    void UnreadData(char *data, size_t len);
    size_t WriteData(char const *data, size_t len);

    void SetWindowSize(ivec2 size, int64_t fd = -1);

private:
    int64_t m_fd;
    int64_t m_pid;
    bool m_eof;
    char const *m_argv[2];
    char *m_unread_data;
    size_t m_unread_len;
    ivec2 m_size;
};

