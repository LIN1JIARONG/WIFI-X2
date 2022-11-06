/*
MIT License

Copyright(c) 2018 Liam Bindle

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <mqtt.h>
#include <stdarg.h>


/* FIXED HEADER */

#define MQTT_BITFIELD_RULE_VIOLOATION(bitfield, rule_value, rule_mask) ((bitfield ^ rule_value) & rule_mask)

struct {
    const uint8_t control_type_is_valid[16];
    const uint8_t required_flags[16];
    const uint8_t mask_required_flags[16];
} mqtt_fixed_header_rules = {
    {   /* boolean value, true if type is valid */
        0x00, /* MQTT_CONTROL_RESERVED */
        0x01, /* MQTT_CONTROL_CONNECT */
        0x01, /* MQTT_CONTROL_CONNACK */
        0x01, /* MQTT_CONTROL_PUBLISH */
        0x01, /* MQTT_CONTROL_PUBACK */
        0x01, /* MQTT_CONTROL_PUBREC */
        0x01, /* MQTT_CONTROL_PUBREL */
        0x01, /* MQTT_CONTROL_PUBCOMP */
        0x01, /* MQTT_CONTROL_SUBSCRIBE */
        0x01, /* MQTT_CONTROL_SUBACK */
        0x01, /* MQTT_CONTROL_UNSUBSCRIBE */
        0x01, /* MQTT_CONTROL_UNSUBACK */
        0x01, /* MQTT_CONTROL_PINGREQ */
        0x01, /* MQTT_CONTROL_PINGRESP */
        0x01, /* MQTT_CONTROL_DISCONNECT */
        0x00  /* MQTT_CONTROL_RESERVED */
    },
    {   /* flags that must be set for the associated control type */
        0x00, /* MQTT_CONTROL_RESERVED */
        0x00, /* MQTT_CONTROL_CONNECT */
        0x00, /* MQTT_CONTROL_CONNACK */
        0x00, /* MQTT_CONTROL_PUBLISH */
        0x00, /* MQTT_CONTROL_PUBACK */
        0x00, /* MQTT_CONTROL_PUBREC */
        0x02, /* MQTT_CONTROL_PUBREL */
        0x00, /* MQTT_CONTROL_PUBCOMP */
        0x02, /* MQTT_CONTROL_SUBSCRIBE */
        0x00, /* MQTT_CONTROL_SUBACK */
        0x02, /* MQTT_CONTROL_UNSUBSCRIBE */
        0x00, /* MQTT_CONTROL_UNSUBACK */
        0x00, /* MQTT_CONTROL_PINGREQ */
        0x00, /* MQTT_CONTROL_PINGRESP */
        0x00, /* MQTT_CONTROL_DISCONNECT */
        0x00  /* MQTT_CONTROL_RESERVED */
    },
    {   /* mask of flags that must be specific values for the associated control type*/
        0x00, /* MQTT_CONTROL_RESERVED */
        0x0F, /* MQTT_CONTROL_CONNECT */
        0x0F, /* MQTT_CONTROL_CONNACK */
        0x00, /* MQTT_CONTROL_PUBLISH */
        0x0F, /* MQTT_CONTROL_PUBACK */
        0x0F, /* MQTT_CONTROL_PUBREC */
        0x0F, /* MQTT_CONTROL_PUBREL */
        0x0F, /* MQTT_CONTROL_PUBCOMP */
        0x0F, /* MQTT_CONTROL_SUBSCRIBE */
        0x0F, /* MQTT_CONTROL_SUBACK */
        0x0F, /* MQTT_CONTROL_UNSUBSCRIBE */
        0x0F, /* MQTT_CONTROL_UNSUBACK */
        0x0F, /* MQTT_CONTROL_PINGREQ */
        0x0F, /* MQTT_CONTROL_PINGRESP */
        0x0F, /* MQTT_CONTROL_DISCONNECT */
        0x00  /* MQTT_CONTROL_RESERVED */
    }
};

ssize_t mqtt_fixed_header_rule_violation(const struct mqtt_fixed_header *fixed_header) {
    uint8_t control_type;
    uint8_t control_flags;
    uint8_t required_flags;
    uint8_t mask_required_flags;

    /* get value and rules */
    control_type = fixed_header->control_type;
    control_flags = fixed_header->control_flags;
    required_flags = mqtt_fixed_header_rules.required_flags[control_type];
    mask_required_flags = mqtt_fixed_header_rules.mask_required_flags[control_type];

    /* check for valid type */
    if (!mqtt_fixed_header_rules.control_type_is_valid[control_type]) {
        return MQTT_ERROR_CONTROL_FORBIDDEN_TYPE;
    }

    /* check that flags are appropriate */
    if(MQTT_BITFIELD_RULE_VIOLOATION(control_flags, required_flags, mask_required_flags)) {
        return MQTT_ERROR_CONTROL_INVALID_FLAGS;
    }

    return 0;
}

ssize_t mqtt_unpack_fixed_header(struct mqtt_response *response, const uint8_t *buf, size_t bufsz) {
    struct mqtt_fixed_header *fixed_header;
    const uint8_t *start = buf;
    int lshift;
    ssize_t errcode;

    /* check for null pointers or empty buffer */
    if (response == NULL || buf == NULL) {
        return MQTT_ERROR_NULLPTR;
    }
    fixed_header = &(response->fixed_header);

    /* check that bufsz is not zero */
    if (bufsz == 0) return 0;

    /* parse control type and flags */
    fixed_header->control_type  = (*buf >> 4);
    fixed_header->control_flags = (*buf & 0x0F);

    /* parse remaining size */
    fixed_header->remaining_length = 0;

    lshift = 0;
    do {

        /* MQTT spec (2.2.3) says the maximum length is 28 bits */
        if(lshift == 28)
            return MQTT_ERROR_INVALID_REMAINING_LENGTH;

        /* consume byte and assert at least 1 byte left */
        --bufsz;
        ++buf;
        if (bufsz == 0) return 0;

        /* parse next byte*/
        fixed_header->remaining_length += (*buf & 0x7F) << lshift;
        lshift += 7;
    } while(*buf & 0x80); /* while continue bit is set */

    /* consume last byte */
    --bufsz;
    ++buf;

    /* check that the fixed header is valid */
    errcode = mqtt_fixed_header_rule_violation(fixed_header);
    if (errcode) {
        return errcode;
    }

    /* check that the buffer size if GT remaining length */
    if (bufsz < fixed_header->remaining_length) {
        return 0;
    }

    /* return how many bytes were consumed */
    return buf - start;
}

ssize_t mqtt_pack_fixed_header(uint8_t *buf, size_t bufsz, const struct mqtt_fixed_header *fixed_header) {
    const uint8_t *start = buf;
    ssize_t errcode;
    uint32_t remaining_length;

    /* check for null pointers or empty buffer */
    if (fixed_header == NULL || buf == NULL) {
        return MQTT_ERROR_NULLPTR;
    }

    /* check that the fixed header is valid */
    errcode = mqtt_fixed_header_rule_violation(fixed_header);
    if (errcode) {
        return errcode;
    }

    /* check that bufsz is not zero */
    if (bufsz == 0) return 0;

    /* pack control type and flags */
    *buf =  (((uint8_t) fixed_header->control_type) << 4) & 0xF0;
    *buf |= ((uint8_t) fixed_header->control_flags)       & 0x0F;

    remaining_length = fixed_header->remaining_length;

    /* MQTT spec (2.2.3) says maximum remaining length is 2^28-1 */
    if(remaining_length >= 256*1024*1024)
        return MQTT_ERROR_INVALID_REMAINING_LENGTH;

    do {
        /* consume byte and assert at least 1 byte left */
        --bufsz;
        ++buf;
        if (bufsz == 0) return 0;

        /* pack next byte */
        *buf  = remaining_length & 0x7F;
        if(remaining_length > 127) *buf |= 0x80;
        remaining_length = remaining_length >> 7;
    } while(*buf & 0x80);

    /* consume last byte */
    --bufsz;
    ++buf;

    /* check that there's still enough space in buffer for packet */
    if (bufsz < fixed_header->remaining_length) {
        return 0;
    }

    /* return how many bytes were consumed */
    return buf - start;
}

/* CONNECT */
ssize_t mqtt_pack_connection_request(uint8_t* buf, size_t bufsz,
                                     const char* client_id,
                                     const char* will_topic,
                                     const void* will_message,
                                     size_t will_message_size,
                                     const char* user_name,
                                     const char* password,
                                     uint8_t connect_flags,
                                     uint16_t keep_alive)
{
    struct mqtt_fixed_header fixed_header;
    size_t remaining_length;
    const uint8_t *const start = buf;
    ssize_t rv;

    /* pack the fixed headr */
    fixed_header.control_type = MQTT_CONTROL_CONNECT;
    fixed_header.control_flags = 0x00;

    /* calculate remaining length and build connect_flags at the same time */
    connect_flags = connect_flags & ~MQTT_CONNECT_RESERVED;
    remaining_length = 10; /* size of variable header */

    if (client_id == NULL) {
        /* client_id is a mandatory parameter */
        return MQTT_ERROR_CONNECT_NULL_CLIENT_ID;
    } else {
        /* mqtt_string length is strlen + 2 */
        remaining_length += __mqtt_packed_cstrlen(client_id);
    }

    if (will_topic != NULL) {
        uint8_t temp;
        /* there is a will */
        connect_flags |= MQTT_CONNECT_WILL_FLAG;
        remaining_length += __mqtt_packed_cstrlen(will_topic);

        if (will_message == NULL) {
            /* if there's a will there MUST be a will message */
            return MQTT_ERROR_CONNECT_NULL_WILL_MESSAGE;
        }
        remaining_length += 2 + will_message_size; /* size of will_message */

        /* assert that the will QOS is valid (i.e. not 3) */
        temp = connect_flags & 0x18; /* mask to QOS */
        if (temp == 0x18) {
            /* bitwise equality with QoS 3 (invalid)*/
            return MQTT_ERROR_CONNECT_FORBIDDEN_WILL_QOS;
        }
    } else {
        /* there is no will so set all will flags to zero */
        connect_flags &= ~MQTT_CONNECT_WILL_FLAG;
        connect_flags &= ~0x18;
        connect_flags &= ~MQTT_CONNECT_WILL_RETAIN;
    }

    if (user_name != NULL) {
        /* a user name is present */
        connect_flags |= MQTT_CONNECT_USER_NAME;
        remaining_length += __mqtt_packed_cstrlen(user_name);
    } else {
        connect_flags &= ~MQTT_CONNECT_USER_NAME;
    }

    if (password != NULL) {
        /* a password is present */
        connect_flags |= MQTT_CONNECT_PASSWORD;
        remaining_length += __mqtt_packed_cstrlen(password);
    } else {
        connect_flags &= ~MQTT_CONNECT_PASSWORD;
    }

    /* fixed header length is now calculated*/
    fixed_header.remaining_length = remaining_length;

    /* pack fixed header and perform error checks */
    rv = mqtt_pack_fixed_header(buf, bufsz, &fixed_header);
    if (rv <= 0) {
        /* something went wrong */
        return rv;
    }
    buf += rv;
    bufsz -= rv;

    /* check that the buffer has enough space to fit the remaining length */
    if (bufsz < fixed_header.remaining_length) {
        return 0;
    }

    /* pack the variable header */
    *buf++ = 0x00;
    *buf++ = 0x04;
    *buf++ = (uint8_t) 'M';
    *buf++ = (uint8_t) 'Q';
    *buf++ = (uint8_t) 'T';
    *buf++ = (uint8_t) 'T';
    *buf++ = MQTT_PROTOCOL_LEVEL;
    *buf++ = connect_flags;
    buf += __mqtt_pack_uint16(buf, keep_alive);

    /* pack the payload */
    buf += __mqtt_pack_str(buf, client_id);
    if (connect_flags & MQTT_CONNECT_WILL_FLAG) {
        buf += __mqtt_pack_str(buf, will_topic);
        buf += __mqtt_pack_uint16(buf, will_message_size);
        memcpy(buf, will_message, will_message_size);
        buf += will_message_size;
    }
    if (connect_flags & MQTT_CONNECT_USER_NAME) {
        buf += __mqtt_pack_str(buf, user_name);
    }
    if (connect_flags & MQTT_CONNECT_PASSWORD) {
        buf += __mqtt_pack_str(buf, password);
    }

    /* return the number of bytes that were consumed */
    return buf - start;
}

/* CONNACK */
ssize_t mqtt_unpack_connack_response(struct mqtt_response *mqtt_response, const uint8_t *buf) {
    const uint8_t *const start = buf;
    struct mqtt_response_connack *response;

    /* check that remaining length is 2 */
    if (mqtt_response->fixed_header.remaining_length != 2) {
        return MQTT_ERROR_MALFORMED_RESPONSE;
    }

    response = &(mqtt_response->decoded.connack);
    /* unpack */
    if (*buf & 0xFE) {
        /* only bit 1 can be set */
        return MQTT_ERROR_CONNACK_FORBIDDEN_FLAGS;
    } else {
        response->session_present_flag = *buf++;
    }

    if (*buf > 5u) {
        /* only bit 1 can be set */
        return MQTT_ERROR_CONNACK_FORBIDDEN_CODE;
    } else {
        response->return_code = (enum MQTTConnackReturnCode) *buf++;
    }
    return buf - start;
}

/* DISCONNECT */
ssize_t mqtt_pack_disconnect(uint8_t *buf, size_t bufsz) {
    struct mqtt_fixed_header fixed_header;
    fixed_header.control_type = MQTT_CONTROL_DISCONNECT;
    fixed_header.control_flags = 0;
    fixed_header.remaining_length = 0;
    return mqtt_pack_fixed_header(buf, bufsz, &fixed_header);
}

/* PING */
ssize_t mqtt_pack_ping_request(uint8_t *buf, size_t bufsz) {
    struct mqtt_fixed_header fixed_header;
    fixed_header.control_type = MQTT_CONTROL_PINGREQ;
    fixed_header.control_flags = 0;
    fixed_header.remaining_length = 0;
    return mqtt_pack_fixed_header(buf, bufsz, &fixed_header);
}
/* PINGACK */
ssize_t mqtt_pack_ping_requestACK(uint8_t *buf, size_t bufsz) {
    struct mqtt_fixed_header fixed_header;
    fixed_header.control_type = MQTT_CONTROL_PINGRESP;
    fixed_header.control_flags = 0;
    fixed_header.remaining_length = 0;
    return mqtt_pack_fixed_header(buf, bufsz, &fixed_header);
}

/* PUBLISH */
ssize_t mqtt_pack_publish_request(uint8_t *buf, size_t bufsz,
                                  const char* topic_name,
                                  uint16_t packet_id,
                                  void* application_message,
                                  size_t application_message_size,
                                  uint8_t publish_flags)
{
    const uint8_t *const start = buf;
    ssize_t rv;
    struct mqtt_fixed_header fixed_header;
    uint16_t remaining_length;
    uint8_t inspected_qos;

    /* check for null pointers */
    if(buf == NULL || topic_name == NULL) {
        return MQTT_ERROR_NULLPTR;
    }

    /* inspect QoS level */
    inspected_qos = (publish_flags & MQTT_PUBLISH_QOS_MASK) >> 1; /* mask */

    /* build the fixed header */
    fixed_header.control_type = MQTT_CONTROL_PUBLISH;

    /* calculate remaining length */
    remaining_length = __mqtt_packed_cstrlen(topic_name);
    if (inspected_qos > 0) {
        remaining_length += 2;
    }
    remaining_length += application_message_size;
    fixed_header.remaining_length = remaining_length;

    /* force dup to 0 if qos is 0 [Spec MQTT-3.3.1-2] */
    if (inspected_qos == 0) {
        publish_flags &= ~MQTT_PUBLISH_DUP;
    }

    /* make sure that qos is not 3 [Spec MQTT-3.3.1-4] */
    if (inspected_qos == 3) {
        return MQTT_ERROR_PUBLISH_FORBIDDEN_QOS;
    }
    fixed_header.control_flags = publish_flags;

    /* pack fixed header */
    rv = mqtt_pack_fixed_header(buf, bufsz, &fixed_header);
    if (rv <= 0) {
        /* something went wrong */
        return rv;
    }
    buf += rv;
    bufsz -= rv;

    /* check that buffer is big enough */
    if (bufsz < remaining_length) {
        return 0;
    }

    /* pack variable header */
    buf += __mqtt_pack_str(buf, topic_name);
    if (inspected_qos > 0) {
        buf += __mqtt_pack_uint16(buf, packet_id);
    }

    /* pack payload */
    memcpy(buf, application_message, application_message_size);
    buf += application_message_size;

    return buf - start;
}

ssize_t mqtt_unpack_publish_response(struct mqtt_response *mqtt_response, const uint8_t *buf)
{
    const uint8_t *const start = buf;
    struct mqtt_fixed_header *fixed_header;
    struct mqtt_response_publish *response;

    fixed_header = &(mqtt_response->fixed_header);
    response = &(mqtt_response->decoded.publish);

    /* get flags */
    response->dup_flag = (fixed_header->control_flags & MQTT_PUBLISH_DUP) >> 3;
    response->qos_level = (fixed_header->control_flags & MQTT_PUBLISH_QOS_MASK) >> 1;
    response->retain_flag = fixed_header->control_flags & MQTT_PUBLISH_RETAIN;

    /* make sure that remaining length is valid */
    if (mqtt_response->fixed_header.remaining_length < 4) {
        return MQTT_ERROR_MALFORMED_RESPONSE;
    }

    /* parse variable header */
    response->topic_name_size = __mqtt_unpack_uint16(buf);
    buf += 2;
    response->topic_name = buf;
    buf += response->topic_name_size;

    if (response->qos_level > 0) {
        response->packet_id = __mqtt_unpack_uint16(buf);
        buf += 2;
    }

    /* get payload */
    response->application_message = buf;
    if (response->qos_level == 0) {
        response->application_message_size = fixed_header->remaining_length - response->topic_name_size - 2;
    } else {
        response->application_message_size = fixed_header->remaining_length - response->topic_name_size - 4;
    }
    buf += response->application_message_size;

    /* return number of bytes consumed */
    return buf - start;
}

/* PUBXXX */
ssize_t mqtt_pack_pubxxx_request(uint8_t *buf, size_t bufsz,
                                 enum MQTTControlPacketType control_type,
                                 uint16_t packet_id)
{
    const uint8_t *const start = buf;
    struct mqtt_fixed_header fixed_header;
    ssize_t rv;
    if (buf == NULL) {
        return MQTT_ERROR_NULLPTR;
    }

    /* pack fixed header */
    fixed_header.control_type = control_type;
    if (control_type == MQTT_CONTROL_PUBREL) {
        fixed_header.control_flags = 0x02;
    } else {
        fixed_header.control_flags = 0;
    }
    fixed_header.remaining_length = 2;
    rv = mqtt_pack_fixed_header(buf, bufsz, &fixed_header);
    if (rv <= 0) {
        return rv;
    }
    buf += rv;
    bufsz -= rv;

    if (bufsz < fixed_header.remaining_length) {
        return 0;
    }

    buf += __mqtt_pack_uint16(buf, packet_id);

    return buf - start;
}

ssize_t mqtt_unpack_pubxxx_response(struct mqtt_response *mqtt_response, const uint8_t *buf)
{
    const uint8_t *const start = buf;
    uint16_t packet_id;

    /* assert remaining length is correct */
    if (mqtt_response->fixed_header.remaining_length != 2) {
        return MQTT_ERROR_MALFORMED_RESPONSE;
    }

    /* parse packet_id */
    packet_id = __mqtt_unpack_uint16(buf);
    buf += 2;

    if (mqtt_response->fixed_header.control_type == MQTT_CONTROL_PUBACK) {
        mqtt_response->decoded.puback.packet_id = packet_id;
    } else if (mqtt_response->fixed_header.control_type == MQTT_CONTROL_PUBREC) {
        mqtt_response->decoded.pubrec.packet_id = packet_id;
    } else if (mqtt_response->fixed_header.control_type == MQTT_CONTROL_PUBREL) {
        mqtt_response->decoded.pubrel.packet_id = packet_id;
    } else {
        mqtt_response->decoded.pubcomp.packet_id = packet_id;
    }

    return buf - start;
}

/* SUBACK */
ssize_t mqtt_unpack_suback_response (struct mqtt_response *mqtt_response, const uint8_t *buf) {
    const uint8_t *const start = buf;
    uint32_t remaining_length = mqtt_response->fixed_header.remaining_length;

    /* assert remaining length is at least 3 (for packet id and at least 1 topic) */
    if (remaining_length < 3) {
        return MQTT_ERROR_MALFORMED_RESPONSE;
    }

    /* unpack packet_id */
    mqtt_response->decoded.suback.packet_id = __mqtt_unpack_uint16(buf);
    buf += 2;
    remaining_length -= 2;

    /* unpack return codes */
    mqtt_response->decoded.suback.num_return_codes = (size_t) remaining_length;
    mqtt_response->decoded.suback.return_codes = buf;
    buf += remaining_length;

    return buf - start;
}

/* SUBSCRIBE */
ssize_t mqtt_pack_subscribe_request(uint8_t *buf, size_t bufsz, unsigned int packet_id, ...) {
    va_list args;
    const uint8_t *const start = buf;
    ssize_t rv;
    struct mqtt_fixed_header fixed_header;
    unsigned int num_subs = 0;
    unsigned int i;
    const char *topic[MQTT_SUBSCRIBE_REQUEST_MAX_NUM_TOPICS];
    uint8_t max_qos[MQTT_SUBSCRIBE_REQUEST_MAX_NUM_TOPICS];

    /* parse all subscriptions */
    va_start(args, packet_id);
    while(1) {
        topic[num_subs] = va_arg(args, const char*);
        if (topic[num_subs] == NULL) {
            /* end of list */
            break;
        }

        max_qos[num_subs] = (uint8_t) va_arg(args, unsigned int);

        ++num_subs;
        if (num_subs >= MQTT_SUBSCRIBE_REQUEST_MAX_NUM_TOPICS) {
            return MQTT_ERROR_SUBSCRIBE_TOO_MANY_TOPICS;
        }
    }
    va_end(args);

    /* build the fixed header */
    fixed_header.control_type = MQTT_CONTROL_SUBSCRIBE;
    fixed_header.control_flags = 2u;
    fixed_header.remaining_length = 2u; /* size of variable header */
    for(i = 0; i < num_subs; ++i) {
        /* payload is topic name + max qos (1 byte) */
        fixed_header.remaining_length += __mqtt_packed_cstrlen(topic[i]) + 1;
    }

    /* pack the fixed header */
    rv = mqtt_pack_fixed_header(buf, bufsz, &fixed_header);
    if (rv <= 0) {
        return rv;
    }
    buf += rv;
    bufsz -= rv;

    /* check that the buffer has enough space */
    if (bufsz < fixed_header.remaining_length) {
        return 0;
    }


    /* pack variable header */
    buf += __mqtt_pack_uint16(buf, packet_id);


    /* pack payload */
    for(i = 0; i < num_subs; ++i) {
        buf += __mqtt_pack_str(buf, topic[i]);
        *buf++ = max_qos[i];
    }

    return buf - start;
}

/* UNSUBACK */
ssize_t mqtt_unpack_unsuback_response(struct mqtt_response *mqtt_response, const uint8_t *buf)
{
    const uint8_t *const start = buf;

    if (mqtt_response->fixed_header.remaining_length != 2) {
        return MQTT_ERROR_MALFORMED_RESPONSE;
    }

    /* parse packet_id */
    mqtt_response->decoded.unsuback.packet_id = __mqtt_unpack_uint16(buf);
    buf += 2;

    return buf - start;
}

/* UNSUBSCRIBE */
ssize_t mqtt_pack_unsubscribe_request(uint8_t *buf, size_t bufsz, unsigned int packet_id, ...) {
    va_list args;
    const uint8_t *const start = buf;
    ssize_t rv;
    struct mqtt_fixed_header fixed_header;
    unsigned int num_subs = 0;
    unsigned int i;
    const char *topic[MQTT_UNSUBSCRIBE_REQUEST_MAX_NUM_TOPICS];

    /* parse all subscriptions */
    va_start(args, packet_id);
    while(1) {
        topic[num_subs] = va_arg(args, const char*);
        if (topic[num_subs] == NULL) {
            /* end of list */
            break;
        }

        ++num_subs;
        if (num_subs >= MQTT_UNSUBSCRIBE_REQUEST_MAX_NUM_TOPICS) {
            return MQTT_ERROR_UNSUBSCRIBE_TOO_MANY_TOPICS;
        }
    }
    va_end(args);

    /* build the fixed header */
    fixed_header.control_type = MQTT_CONTROL_UNSUBSCRIBE;
    fixed_header.control_flags = 2u;
    fixed_header.remaining_length = 2u; /* size of variable header */
    for(i = 0; i < num_subs; ++i) {
        /* payload is topic name */
        fixed_header.remaining_length += __mqtt_packed_cstrlen(topic[i]);
    }

    /* pack the fixed header */
    rv = mqtt_pack_fixed_header(buf, bufsz, &fixed_header);
    if (rv <= 0) {
        return rv;
    }
    buf += rv;
    bufsz -= rv;

    /* check that the buffer has enough space */
    if (bufsz < fixed_header.remaining_length) {
        return 0;
    }

    /* pack variable header */
    buf += __mqtt_pack_uint16(buf, packet_id);


    /* pack payload */
    for(i = 0; i < num_subs; ++i) {
        buf += __mqtt_pack_str(buf, topic[i]);
    }

    return buf - start;
}

/* MESSAGE QUEUE */
void mqtt_mq_init(struct mqtt_message_queue *mq, void *buf, size_t bufsz)
{
    mq->mem_start = buf;
    mq->mem_end = (unsigned char*)buf + bufsz;
    mq->curr = buf;
    mq->queue_tail = mq->mem_end;
    mq->curr_sz = mqtt_mq_currsz(mq);
}

struct mqtt_queued_message* mqtt_mq_register(struct mqtt_message_queue *mq, size_t nbytes)
{
    /* make queued message header */
    --(mq->queue_tail);
    mq->queue_tail->start = mq->curr;
    mq->queue_tail->size = nbytes;
    mq->queue_tail->state = MQTT_QUEUED_UNSENT;

    /* move curr and recalculate curr_sz */
    mq->curr += nbytes;
    mq->curr_sz = mqtt_mq_currsz(mq);

    return mq->queue_tail;
}

void mqtt_mq_clean(struct mqtt_message_queue *mq) {
    struct mqtt_queued_message *new_head;

    for(new_head = mqtt_mq_get(mq, 0); new_head >= mq->queue_tail; --new_head) {
        if (new_head->state != MQTT_QUEUED_COMPLETE) break;
    }

    /* check if everything can be removed */
    if (new_head < mq->queue_tail) {
        mq->curr = mq->mem_start;
        mq->queue_tail = mq->mem_end;
        mq->curr_sz = mqtt_mq_currsz(mq);
        return;
    } else if (new_head == mqtt_mq_get(mq, 0)) {
        /* do nothing */
        return;
    }

    /* move buffered data */
    {
        size_t n = mq->curr - new_head->start;
        size_t removing = new_head->start - (uint8_t*) mq->mem_start;
        memmove(mq->mem_start, new_head->start, n);
        mq->curr = (unsigned char*)mq->mem_start + n;


        /* move queue */
        {
            ssize_t new_tail_idx = new_head - mq->queue_tail;
            memmove(mqtt_mq_get(mq, new_tail_idx), mq->queue_tail, sizeof(struct mqtt_queued_message) * (new_tail_idx + 1));
            mq->queue_tail = mqtt_mq_get(mq, new_tail_idx);

            {
                /* bump back start's */
                ssize_t i = 0;
                for(; i < new_tail_idx + 1; ++i) {
                    mqtt_mq_get(mq, i)->start -= removing;
                }
            }
        }
    }

    /* get curr_sz */
    mq->curr_sz = mqtt_mq_currsz(mq);
}

struct mqtt_queued_message* mqtt_mq_find(struct mqtt_message_queue *mq, enum MQTTControlPacketType control_type, uint16_t *packet_id)
{
    struct mqtt_queued_message *curr;
    for(curr = mqtt_mq_get(mq, 0); curr >= mq->queue_tail; --curr) {
        if (curr->control_type == control_type) {
            if ((packet_id == NULL && curr->state != MQTT_QUEUED_COMPLETE) ||
                (packet_id != NULL && *packet_id == curr->packet_id)) {
                return curr;
            }
        }
    }
    return NULL;
}


/* RESPONSE UNPACKING */
ssize_t mqtt_unpack_response(struct mqtt_response* response, const uint8_t *buf, size_t bufsz) {
    const uint8_t *const start = buf;
    ssize_t rv = mqtt_unpack_fixed_header(response, buf, bufsz);
    if (rv <= 0) return rv;
    else buf += rv;
    switch(response->fixed_header.control_type) {
        case MQTT_CONTROL_CONNACK:
            rv = mqtt_unpack_connack_response(response, buf);
            break;
        case MQTT_CONTROL_PUBLISH:
            rv = mqtt_unpack_publish_response(response, buf);
            break;
        case MQTT_CONTROL_PUBACK:
            rv = mqtt_unpack_pubxxx_response(response, buf);
            break;
        case MQTT_CONTROL_PUBREC:
            rv = mqtt_unpack_pubxxx_response(response, buf);
            break;
        case MQTT_CONTROL_PUBREL:
            rv = mqtt_unpack_pubxxx_response(response, buf);
            break;
        case MQTT_CONTROL_PUBCOMP:
            rv = mqtt_unpack_pubxxx_response(response, buf);
            break;
        case MQTT_CONTROL_SUBACK:
            rv = mqtt_unpack_suback_response(response, buf);
            break;
        case MQTT_CONTROL_UNSUBACK:
            rv = mqtt_unpack_unsuback_response(response, buf);
            break;
        case MQTT_CONTROL_PINGRESP:
            return rv;
        default:
            return MQTT_ERROR_RESPONSE_INVALID_CONTROL_TYPE;
    }

    if (rv < 0) return rv;
    buf += rv;
    return buf - start;
}

/* EXTRA DETAILS */
ssize_t __mqtt_pack_uint16(uint8_t *buf, uint16_t integer)
{
//  uint16_t integer_htons = MQTT_PAL_HTONS(integer);
//  memcpy(buf, &integer_htons, 2);

	buf[0] = (uint8_t)((integer>>8)&0x00FF); // LI 2019.08.21
	buf[1] = (uint8_t)((integer)&0x00FF);

  return 2;
}

uint16_t __mqtt_unpack_uint16(const uint8_t *buf)
{
  uint16_t integer_htons=0;
//  memcpy(&integer_htons, buf, 2);
//	return MQTT_PAL_NTOHS(integer_htons);

	integer_htons = buf[0];  // LI 2019.08.21
	integer_htons <<= 8;
	integer_htons |= buf[1];

  return integer_htons;
}

ssize_t __mqtt_pack_str(uint8_t *buf, const char* str) {
    uint16_t length = strlen(str);
    int i = 0;
     /* pack string length */
    buf += __mqtt_pack_uint16(buf, length);

    /* pack string */
    for(; i < length; ++i) {
        *(buf++) = str[i];
    }

    /* return number of bytes consumed */
    return length + 2;
}

static const char *MQTT_ERRORS_STR[] = {
    "MQTT_UNKNOWN_ERROR",
    __ALL_MQTT_ERRORS(GENERATE_STRING)
};

const char* mqtt_error_str(enum MQTTErrors error) {
    int offset = error - MQTT_ERROR_UNKNOWN;
    if (offset >= 0) {
        return MQTT_ERRORS_STR[offset];
    } else if (error == 0) {
        return "MQTT_ERROR: Buffer too small.";
    } else if (error > 0) {
        return "MQTT_OK";
    } else {
        return MQTT_ERRORS_STR[0];
    }
}

/** @endcond*/
