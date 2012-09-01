//
// Neercs
//

#if !defined __TERM_PTY_H__
#define __TERM_PTY_H__

class Pty
{
public:
    Pty(ivec2 size);
    ~Pty();

    void Run(char const *command);
    void SetWindowSize(ivec2 size);

private:
    uint64_t m_fd;
    uint64_t m_pid;
    char const *m_argv[2];
    ivec2 m_size;
};

#endif // __TERM_PTY_H__

