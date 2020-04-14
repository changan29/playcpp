
#ifndef _AI_MEMPOOL_H
#define _AI_MEMPOOL_H

#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>

const size_t MAX_EXTEND_LEN = 1024;

class MemPool
{
protected:
    static const size_t INVALID_POINTER = static_cast<size_t>(-1);

public:
    typedef struct AllocBlock
    {
        size_t next;        // 该值如果为0则表示为已分配使用的Block,否则next值用来链接空闲链,INVALID_POINTER表示空闲链结尾
        char node[0];
    } AllocBlock;

    static const size_t NODE_OFFSET = offsetof(AllocBlock, node);

    typedef struct MemHeader
    {
        size_t max_size;        // 最大可分配大小
        size_t alloc_size;      // 已分配大小
        size_t node_size;       // 节点实际大小
        size_t block_size;      // 分配块大小
        size_t free_list;       // 空闲链
        char   extend[MAX_EXTEND_LEN];    // 增加1k作为扩展字段，存储其他信息。
    } MemHeader;

    MemPool(): header_(NULL), data_(NULL)
    {
    }

    virtual ~MemPool()
    {
    }

    size_t Ref(void * p) const
    {
        return (size_t)((intptr_t)p - (intptr_t)(header_));
    }

    void * Deref(size_t pos) const
    {
        return ((char *)header_ + pos);
    }

    int Init(key_t key, size_t max_size, size_t node_size, bool & exist)
    {
        exist = false;

        int shmid = shmget(key, max_size, IPC_CREAT | IPC_EXCL | 0644);
        if (shmid < 0)
        {
            if (errno != EEXIST)
            {
                snprintf(error_msg_, sizeof(error_msg_), "MemPool.Init shmget 共享内存失败,key=%d,size=%zu,errno=%d,msg=%s",
                    key, max_size, errno, strerror(errno));
                return -1;
            }
            exist = true;

            shmid = shmget(key, max_size, 0644);
            if (shmid < 0)
            {
                snprintf(error_msg_, sizeof(error_msg_), "MemPool.Init shmget 已存在的共享内存失败,key=%d,size=%zu,"
                    "errno=%d,msg=%s", key, max_size, errno, strerror(errno));
                return -1;
            }
        }

        void * shm = shmat(shmid, NULL, 0);
        if (!shm)
        {
            snprintf(error_msg_, sizeof(error_msg_), "MemPool.Init shmat attach共享内存失败 shmid=%d,key=%d,size=%zu,"
                "errno=%d,msg=%s", shmid, key, max_size, errno, strerror(errno));
            return -1;
        }

        return Init(shm, max_size, node_size, exist);
    }

    int Init(void * mem, size_t max_size, size_t node_size, bool recovery = false)
    {
        if (!mem)
        {
            return -1;
        }

        header_ = (MemHeader *)mem;

        if (recovery)
        {
            // 验证内存头部信息是否正确
            if (header_->max_size != max_size)
            {
                snprintf(error_msg_, sizeof(error_msg_), "MemPool.Init invalid memory header %p,"
                    "header_->max_size=%zu,max_size=%zu", mem, header_->max_size, max_size);
                return -1;
            }
            if (header_->alloc_size > max_size)
            {
                snprintf(error_msg_, sizeof(error_msg_), "MemPool.Init invalid memory header %p,"
                    "header_->alloc_size=%zu,max_size=%zu", mem, header_->alloc_size, max_size);
                return -1;
            }
            if (header_->node_size != node_size)
            {
                snprintf(error_msg_, sizeof(error_msg_), "MemPool.Init invalid memory header %p,"
                    "header_->node_size=%zu,node_size=%zu", mem, header_->node_size, node_size);
                return -1;
            }
            if (header_->block_size != GetBlockSize(node_size))
            {
                snprintf(error_msg_, sizeof(error_msg_), "MemPool.Init invalid memory header %p,"
                    "header_->block_size=%zu,node_size=%zu", mem, header_->block_size, node_size);
                return -1;
            }

            data_ = (char *)header_ + sizeof(MemHeader);

            return RecoveryData();
        }
        else
        {
            // 初始化内存头
            header_->max_size = max_size;
            header_->alloc_size = sizeof(MemHeader);
            header_->node_size = node_size;
            header_->block_size = GetBlockSize(node_size);
            header_->free_list = INVALID_POINTER;
            //
            memset(header_->extend, 0, MAX_EXTEND_LEN);

            data_ = (char *)header_ + sizeof(MemHeader);

            return 0;
        }
    }

    void * Alloc(bool zero = true)
    {
        AllocBlock * p = GetFreeBlock();
        if (!p)
        {
            // 没有找到空闲块，需要从池中分配内存
            if (header_->alloc_size + header_->block_size <= header_->max_size)
            {
                p = (AllocBlock *)Deref(header_->alloc_size);
                header_->alloc_size += header_->block_size;
            }
        }

        if (p)
        {
            if (zero) memset(p, 0, header_->block_size);
            p->next = 0;
            return ((char *)p + NODE_OFFSET);
        }
        else
        {
            return NULL;
        }
    }

    int Free(void * node)
    {
        AllocBlock * block = (AllocBlock *)((char *)node - NODE_OFFSET);
        if (!ValidBlock(block))
        {
            snprintf(error_msg_, sizeof(error_msg_), "MemPool.Free 释放指针错误,指针%p为非法指针,"
                "header=%p,max_size=%zu,alloc_size=%zu,block_size=%zu", node, header_,
                header_->max_size, header_->alloc_size, header_->block_size);
            return -1;
        }

        LinkFreeBlock(block);

        return 0;
    }

    char* GetExtendBuf(size_t& extend_size) 
    {
        extend_size = MAX_EXTEND_LEN;
        return header_->extend;
    }


    const MemHeader * Header() const
    {
        return header_;
    }

    const char * GetErrMsg() const
    {
        return error_msg_;
    }

    size_t GetBlockSize(size_t node_size) const
    {
        // 分配块大小按照8字节对齐
        return ((node_size + NODE_OFFSET + 7) & ~7);
    }

    size_t GetTotalSize(size_t node_size, size_t node_num) const
    {
            return sizeof(MemHeader) + GetBlockSize(node_size) * node_num;
    }

    const AllocBlock * GetFirstBlock() const
    {
        const AllocBlock * block = (AllocBlock *)data_;
        return (ValidBlock(block) ? block : NULL);
    }

    const AllocBlock * GetNextBlock(const AllocBlock * block)  const
    {
        const AllocBlock * next = (AllocBlock *)((char *)block + header_->block_size);
        return (ValidBlock(next) ? next : NULL);
    }

protected:

    bool ValidBlock(const AllocBlock * block) const
    {
        return !(block < data_ || (size_t)block + header_->block_size > (size_t)header_ + header_->alloc_size ||
            Ref(const_cast<AllocBlock*>(block)) % header_->block_size != sizeof(MemHeader) % header_->block_size);
    }

    void LinkFreeBlock(AllocBlock * p)
    {
        p->next = header_->free_list;
        header_->free_list = Ref(p);
    }

    AllocBlock * GetFreeBlock()
    {
        AllocBlock * p = NULL;
        if (header_->free_list == INVALID_POINTER)
        {
            p = NULL;
        }
        else
        {
            p = (AllocBlock *)Deref(header_->free_list);
            header_->free_list = p->next;
        }

        return p;
    }

    virtual int RecoveryData()
    {
        return 0;
    }

protected:
    MemHeader * header_;
    void * data_;
    char error_msg_[256];
};


#endif

