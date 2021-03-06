

/*
 * USB ring definition
 */

typedef struct {
    uint8_t* data;
    uint16_t length;
    uint16_t current;
} usb_buffer_t;

#define USB_RING_CAPACITY 4
typedef struct {
    usb_buffer_t buffers[USB_RING_CAPACITY];
    int begin;
    int size;
    int nfreed;
} usb_ring_t;

/*
 * USB ring implemetation
 */

static void usbring_init(volatile usb_ring_t* ring) {
    ring->begin = 0;
    ring->size = 0;
    ring->nfreed = 0;
}

static int usbring_post_buffer(volatile usb_ring_t* ring, uint8_t* buffer, uint16_t length) {
    volatile usb_buffer_t* buf;
    if (ring->size + ring->nfreed == USB_RING_CAPACITY) {
        return 0;
    }
    buf = &ring->buffers[(ring->begin + ring->size) & (USB_RING_CAPACITY - 1)];
    buf->data = buffer;
    buf->length = length;
    buf->current = 0;
    
    ring->size++;

    return 1;    
}

static void usbring_free_buffer(volatile usb_ring_t* ring) {
    ring->begin = (ring->begin + 1) & (USB_RING_CAPACITY - 1);
    ring->size--;
    ring->nfreed++;
}

static int usbring_pop_freed(volatile usb_ring_t* ring) {
    if (ring->nfreed == 0) {
      //LOG_INFO("Buffer not available");
        return 0;
    } else {
        int first = (ring->begin - ring->nfreed) & (USB_RING_CAPACITY - 1);
        volatile usb_buffer_t* buf = &ring->buffers[first];
        int len = buf->current;
        //assert(len > 0);
        ring->nfreed--;
        return len;
    }
}

static void usbring_reset(volatile usb_ring_t* ring) {
    if (ring->size > 0) {
        ring->buffers[ring->begin].current = 0;
    }
}

