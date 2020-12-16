#pragma once

#define AK_FORBID_COPY(cls_name)                                                                   \
  public:                                                                                          \
    cls_name(const cls_name&) = delete;                                                            \
    cls_name& operator=(const cls_name&) = delete;
