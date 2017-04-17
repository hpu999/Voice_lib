#include "source.h"
#include "dma.h"

#include <QTimer>
#include <QReadWriteLock>

namespace DplSource {

class SourcePrivate
{
    Q_DECLARE_PUBLIC(Source)
public:
    SourcePrivate(Source *parent);
    void update_dma();

    /* Attribution */
    mutable QReadWriteLock m_rwlock;

    QTimer m_timer;
    int m_interval;                 // 上传的间隔时间
    bool m_delayFlag;
    Source::Type m_type;

    Dma *m_dmaSource;

private:
    Source * const q_ptr;
};

SourcePrivate::SourcePrivate(Source *parent) :
    q_ptr(parent)
{
    m_interval  = 20;
    m_delayFlag = false;
    m_type      = Source::DMA;

    m_timer.setInterval(m_interval);
    m_dmaSource = Dma::instance();
}

void SourcePrivate::update_dma()
{
    Q_Q(Source);
    const char *data = m_dmaSource->read_data();
    if (data == NULL) {
        return;
    }
    emit q->data_event(data);
}

/* Source */
Source *Source::instance()
{
    static Source *ins = new Source();
    return ins;
}

Source::Type Source::type() const
{
    Q_D(const Source);
    QReadLocker l(&d->m_rwlock);
    return d->m_type;
}

void Source::set_type(Source::Type type)
{
    Q_D(Source);
    QWriteLocker l(&d->m_rwlock);
    d->m_type = type;
}

int Source::interval() const
{
    Q_D(const Source);
    QReadLocker l(&d->m_rwlock);
    return d->m_interval;
}

void Source::set_interval(unsigned int interval)
{
    Q_D(Source);
    QWriteLocker l(&d->m_rwlock);
    d->m_interval = interval;
    d->m_timer.setInterval(d->m_interval);
}

void Source::stop()
{
    Q_D(Source);
    QWriteLocker l(&d->m_rwlock);
    d->m_timer.stop();
}

bool Source::is_running() const
{
    Q_D(const Source);
    QReadLocker l(&d->m_rwlock);
    return d->m_timer.isActive();
}

void Source::start()
{
    Q_D(Source);
    QWriteLocker l(&d->m_rwlock);
    d->m_timer.start();
}

void Source::restart()
{
    Q_D(Source);
    d->m_delayFlag = true;
    d->m_timer.start(200);
}

Source::Source()
    : d_ptr(new SourcePrivate(this))
{
    connect(&d_ptr->m_timer, SIGNAL(timeout()), this, SLOT(update()));
}

Source::~Source()
{
    delete d_ptr;
}

void Source::update()
{
    Q_D(Source);
    Type type;

    {
        QWriteLocker l(&d->m_rwlock);
        type = d->m_type;

        if (d->m_delayFlag) {
            d->m_delayFlag = false;
            d->m_timer.setInterval(d->m_interval);
            return;
        }
    }

    if (DMA == type) {
        d->update_dma();
    }
}

}