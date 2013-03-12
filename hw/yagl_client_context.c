#include "yagl_client_context.h"
#include "yagl_sharegroup.h"

void yagl_client_context_init(struct yagl_client_context *ctx,
                              yagl_client_api client_api,
                              struct yagl_sharegroup *sg)
{
    ctx->client_api = client_api;
    yagl_sharegroup_acquire(sg);
    ctx->sg = sg;
}

void yagl_client_context_cleanup(struct yagl_client_context *ctx)
{
    yagl_sharegroup_release(ctx->sg);
}
